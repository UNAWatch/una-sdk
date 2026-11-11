/**
 ******************************************************************************
 * @file    OutdoorStrideCalibrator.cpp
 * @brief   Implementation of the outdoor stride calibrator.
 ******************************************************************************
 */

#include "SDK/Calibration/OutdoorStrideCalibrator.hpp"

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>

#include "SDK/Calibration/StrideMath.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

namespace SDK::Calibration
{

OutdoorStrideCalibrator::OutdoorStrideCalibrator(SDK::Interface::IFileSystem &fs,
                                                 const char *path) :
    mFs(fs)
{
    // Copy the path; it is used across the session (load/finalise/backup) and
    // must not depend on the caller's argument lifetime.
    const char *src = (path != nullptr) ? path : kDefaultPath;
    const int written = std::snprintf(mPath, sizeof(mPath), "%s", src);
    if (written < 0 || written >= static_cast<int>(sizeof(mPath))) {
        // Path too long to store safely — fall back to the (short) default so
        // load/finalise never target a silently-truncated, wrong path.
        std::snprintf(mPath, sizeof(mPath), "%s", kDefaultPath);
    }
}

OutdoorStrideCalibrator::~OutdoorStrideCalibrator()
{
    closeTrace();
}

// --- Bin layout helpers ------------------------------------------------------
// Delegated to StrideLut so the calibrator (write) and the model (read) share
// one bin↔cadence mapping.

float OutdoorStrideCalibrator::binCentreSpm(size_t index)
{
    return StrideLut::binCentreSpm(index);
}

size_t OutdoorStrideCalibrator::binIndexForCadence(float cadenceSpm)
{
    return StrideLut::binIndexForCadence(cadenceSpm);
}

// --- Steady-state machine -----------------------------------------

void OutdoorStrideCalibrator::resetSteadyState()
{
    mSteadySeconds = 0.0f;
    mRefsSet       = false;
}

void OutdoorStrideCalibrator::ingestSample(const CalibratorSample &s)
{
    if (mPaused) {
        return;
    }

    // Stream gap / time discontinuity: a delta_t jump (> MAX_TICK_GAP_S)
    // OR a non-positive delta_t (a repeated/backward clock, since the app
    // derives delta_t from UTC timestamps) breaks the consecutive-seconds run.
    // A non-positive delta_t is especially dangerous: it would shrink
    // mSteadySeconds and make accumulate() subtract distance/steps. The
    // discontinuity tick must NOT advance the window or integrate distance; it
    // only re-anchors the reference values for the next contiguous tick.
    const bool gapBroke = (s.delta_t_s > Config::kMaxTickGapS) ||
                          (s.delta_t_s <= 0.0f);
    if (gapBroke) {
        resetSteadyState();
    }

    // Evaluate the non-steady-state gates individually so the debug
    // trace can report which gate rejected a tick.
    GateTrace g;
    g.gpsValid    = s.gps_speed_valid;
    g.fixOk       = !s.gps_fix_dead_reckoning;
    g.speedBounds = (s.gps_speed_ms >= Config::kGpsSpeedMinMs &&
                     s.gps_speed_ms <= Config::kGpsSpeedMaxMs);
    g.cadValid    = s.cadence_valid;
    g.cadBounds   = (s.cadence_spm >= Config::kCadenceMinSpm &&
                     s.cadence_spm <= Config::kCadenceMaxSpm);
    g.gradeValid  = s.grade_valid;
    g.gradeBounds = (std::fabs(s.grade_pct) <= Config::kGradeMaxPct);

    const bool nonSteadyPass = g.gpsValid && g.fixOk && g.speedBounds &&
                               g.cadValid && g.cadBounds &&
                               g.gradeValid && g.gradeBounds;

    const char *verdict;

    if (!nonSteadyPass) {
        resetSteadyState();
        verdict = "discard_gate";
    } else {
        // First qualifying tick after a reset latches the reference values
        // They are > 0 here (speed/cadence bound gates passed).
        if (!mRefsSet) {
            mSpeedRef   = s.gps_speed_ms;
            mCadenceRef = s.cadence_spm;
            mRefsSet    = true;
        }

        g.steadyEval  = true;
        g.speedSteady = std::fabs(s.gps_speed_ms - mSpeedRef) / mSpeedRef
                            <= Config::kSteadyBandFrac;
        g.cadSteady   = std::fabs(s.cadence_spm - mCadenceRef) / mCadenceRef
                            <= Config::kSteadyBandFrac;

        if (!(g.speedSteady && g.cadSteady)) {
            resetSteadyState();
            verdict = "discard_steady";
        } else if (gapBroke) {
            // Discontinuity: gates pass and refs are now re-anchored, but the
            // gap tick neither advances the window nor accumulates distance
            // The next contiguous tick starts the fresh window.
            verdict = "discard_gap";
        } else {
            // All gates pass — advance the counter.
            mSteadySeconds += s.delta_t_s;

            if (mSteadySeconds + 1e-4f < Config::kSteadyStateMinSeconds) {
                verdict = "qualifying";  // still qualifying, discard for now
            } else {
                // Post-gate stride-plausibility check. A rejection here
                // does NOT reset the counter (the gates all passed).
                const float slImplied = StrideMath::impliedStrideLengthM(
                    s.gps_speed_ms, s.cadence_spm);
                if (slImplied < Config::kStrideMinM ||
                    slImplied > Config::kStrideMaxM) {
                    verdict = "discard_plausibility";
                } else {
                    accumulate(s);
                    ++mSessionAccepted;
                    verdict = "accept";
                }
            }
        }
    }

    if (mTraceFile) {
        writeTraceRow(s, g, verdict);
    }
}

void OutdoorStrideCalibrator::pause()
{
    mPaused = true;
    resetSteadyState();
}

void OutdoorStrideCalibrator::resume()
{
    mPaused = false;
    resetSteadyState();
}

// --- Bin aggregation --------------------------------------------

void OutdoorStrideCalibrator::accumulate(const CalibratorSample &s)
{
    const size_t i = binIndexForCadence(s.cadence_spm);
    StrideBin   &b = mLut.bin(i);

    const float dSample     = s.gps_speed_ms * s.delta_t_s;
    const float stepsSample = s.cadence_spm * s.delta_t_s / 60.0f;

    // Aging: only when adding this sample would push the bin over the cap.
    // Scale existing totals so the post-add distance lands exactly at the cap,
    // yielding a distance-weighted EMA.
    if (b.total_distance_m + dSample > Config::kDistanceCapM &&
        b.total_distance_m > 0.0f) {
        float scale = (Config::kDistanceCapM - dSample) / b.total_distance_m;
        if (scale < 0.0f) {
            // A single tick's distance exceeds the whole cap (pathological —
            // gap ticks are excluded from accumulation, so dSample stays small).
            // Discard the old data rather than letting the scale go negative.
            scale = 0.0f;
        }
        b.total_distance_m *= scale;
        b.total_steps      *= scale;
        b.sample_count     *= scale;
    }

    b.total_distance_m += dSample;
    b.total_steps      += stepsSample;
    b.sample_count     += 1.0f;
}

// --- Phase-2 gate read path ---------------------------------------
// Forwarded to the shared LUT.

size_t OutdoorStrideCalibrator::validBinCount() const
{
    return mLut.validBinCount();
}

float OutdoorStrideCalibrator::totalCalibrationDistanceM() const
{
    return mLut.totalCalibrationDistanceM();
}

bool OutdoorStrideCalibrator::readyForPhase2() const
{
    return mLut.readyForPhase2();
}

// --- Persistence --------------------------------------------------

void OutdoorStrideCalibrator::backupCorruptFile()
{
    char bak[SDK::Interface::IFileSystem::skMaxPathLen] {};
    const int written = std::snprintf(bak, sizeof(bak), "%s.bak", mPath);
    if (written > 0 && written < static_cast<int>(sizeof(bak))) {
        mFs.copy(mPath, bak);
    }
}

void OutdoorStrideCalibrator::load()
{
    mLut.clear();

    // The calibrator instance is reused across runs; start each session clean
    // so the write gate (accepted-this-session > 0) is per-session.
    mSessionAccepted = 0;
    mPaused          = false;
    resetSteadyState();

    std::unique_ptr<SDK::Interface::IFile> file = mFs.file(mPath);
    if (!file || !file->exist()) {
        return;  // absent → fresh, no backup
    }

    if (!file->open(false, false)) {
        return;  // cannot open → fresh, no backup
    }

    const size_t size = file->size();
    if (size == 0) {
        file->close();
        return;  // empty → fresh, no backup
    }
    if (size > kMaxStoreBytes) {
        // Implausibly large store: treat as corrupt and avoid a large transient
        // heap allocation. Back it up for inspection and start fresh.
        file->close();
        backupCorruptFile();
        return;  // bins already cleared above
    }

    char *buffer = new (std::nothrow) char[size];
    if (buffer == nullptr) {
        file->close();
        return;
    }

    size_t read = 0;
    const bool readOk = file->read(buffer, size, read) && (read == size);
    file->close();

    if (!readOk) {
        delete[] buffer;
        return;  // read error → fresh, no backup
    }

    if (!mLut.parseBuffer(buffer, size)) {
        // Malformed JSON: back up and start fresh.
        backupCorruptFile();
        mLut.clear();
    }

    delete[] buffer;
}

bool OutdoorStrideCalibrator::finalise()
{
    // Session is ending — close the debug trace (if any) regardless of outcome.
    closeTrace();

    // Write condition: only persist if at least one sample was accepted.
    if (mSessionAccepted == 0) {
        return false;
    }

    // Ensure the SharedData directory exists (FatFs f_open does not create
    // missing parents). "Already exists" counts as success.
    const char *slash = std::strrchr(mPath, '/');
    if (slash != nullptr) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
        std::snprintf(dir, sizeof(dir), "%.*s",
                      static_cast<int>(slash - mPath), mPath);
        if (!mFs.mkdir(dir)) {
            return false;
        }
    }

    std::unique_ptr<SDK::Interface::IFile> file = mFs.file(mPath);
    if (!file || !file->open(true, true)) {
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());
    writer.startMap();
    writer.add("version", static_cast<int32_t>(kCurrentVersion));
    writer.startArray("bins");
    for (size_t i = 0; i < Config::kBinCount; ++i) {
        writer.startMap();
        const StrideBin &b = mLut.bin(i);
        writer.add("centre_spm",
                   static_cast<int32_t>(std::lround(binCentreSpm(i))));
        writer.add("total_distance_m", b.total_distance_m);
        writer.add("total_steps",      b.total_steps);
        writer.add("sample_count",     b.sample_count);
        writer.endMap();
    }
    writer.endArray();
    writer.endMap();
    writer.flush();

    // Fold the final flush/close status into the result: on flash-backed FatFs
    // the buffered tail may only reach storage at flush()/close(), so a failure
    // there leaves the persisted store incomplete even when the writer itself
    // reported no error. Call both unconditionally so close() still runs if
    // flush() fails.
    bool ok = !writer.isError();
    ok = file->flush() && ok;
    ok = file->close() && ok;
    return ok;
}

// --- Debug trace ------------------------------------------------

void OutdoorStrideCalibrator::enableTrace(const char *path)
{
    closeTrace();
    if (path == nullptr) {
        return;
    }

    // Ensure the parent directory exists (e.g. SharedData). Ignore the result;
    // the open below is the real success check.
    const char *slash = std::strrchr(path, '/');
    if (slash != nullptr) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
        std::snprintf(dir, sizeof(dir), "%.*s",
                      static_cast<int>(slash - path), path);
        mFs.mkdir(dir);
    }

    std::unique_ptr<SDK::Interface::IFile> file = mFs.file(path);
    if (!file || !file->open(true, true)) {
        return;  // leave tracing disabled on failure
    }

    static const char header[] =
        "row,gps_speed_ms,gps_speed_valid,gps_fix_dead_reckoning,"
        "cadence_spm,cadence_valid,grade_pct,grade_valid,delta_t_s,"
        "g_gps,g_fix,g_speed,g_cad_valid,g_cad_bounds,g_grade_valid,"
        "g_grade_bounds,g_speed_steady,g_cad_steady,steady_s,verdict\n";
    size_t bw = 0;
    if (!file->write(header, sizeof(header) - 1, bw) || !file->flush()) {
        file->close();
        return;  // header write failed → leave tracing disabled
    }

    mTraceFile = std::move(file);
    mTraceRow  = 0;
}

void OutdoorStrideCalibrator::writeTraceRow(const CalibratorSample &s,
                                            const GateTrace &g,
                                            const char *verdict)
{
    if (!mTraceFile) {
        return;
    }

    char line[256];
    const int n = std::snprintf(
        line, sizeof(line),
        "%" PRIu32 ",%.3f,%d,%d,%.2f,%d,%.3f,%d,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%s\n",
        mTraceRow,
        static_cast<double>(s.gps_speed_ms), s.gps_speed_valid ? 1 : 0,
        s.gps_fix_dead_reckoning ? 1 : 0,
        static_cast<double>(s.cadence_spm), s.cadence_valid ? 1 : 0,
        static_cast<double>(s.grade_pct), s.grade_valid ? 1 : 0,
        static_cast<double>(s.delta_t_s),
        g.gpsValid ? 1 : 0, g.fixOk ? 1 : 0, g.speedBounds ? 1 : 0,
        g.cadValid ? 1 : 0, g.cadBounds ? 1 : 0, g.gradeValid ? 1 : 0,
        g.gradeBounds ? 1 : 0, g.speedSteady ? 1 : 0, g.cadSteady ? 1 : 0,
        static_cast<double>(mSteadySeconds), verdict);

    if (n > 0) {
        size_t bw = 0;
        const size_t len = (static_cast<size_t>(n) < sizeof(line))
                               ? static_cast<size_t>(n)
                               : sizeof(line) - 1;
        if (!mTraceFile->write(line, len, bw) || !mTraceFile->flush()) {
            // Disable tracing on I/O failure rather than silently retrying a
            // dead sink every tick (closeTrace() resets mTraceFile).
            closeTrace();
            return;
        }
    }
    ++mTraceRow;
}

void OutdoorStrideCalibrator::closeTrace()
{
    if (mTraceFile) {
        mTraceFile->flush();
        mTraceFile->close();
        mTraceFile.reset();
    }
}

} // namespace SDK::Calibration
