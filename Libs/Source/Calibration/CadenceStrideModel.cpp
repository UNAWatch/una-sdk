/**
 ******************************************************************************
 * @file    CadenceStrideModel.cpp
 * @brief   Implementation of the read-side cadence/stride model.
 ******************************************************************************
 */

#include "SDK/Calibration/CadenceStrideModel.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

namespace SDK::Calibration
{

namespace
{

/// Clamp a value into [lo, hi]. NaN maps to lo (NaN comparisons are all false,
/// so the first branch is taken).
float clampf(float v, float lo, float hi)
{
    if (!(v > lo)) {  // v <= lo or NaN
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/// Copy a path into a fixed buffer, falling back to a default on overflow.
void copyPath(char *dst, size_t dstLen, const char *src, const char *fallback)
{
    const char *p = (src != nullptr) ? src : fallback;
    const int written = std::snprintf(dst, dstLen, "%s", p);
    if (written < 0 || written >= static_cast<int>(dstLen)) {
        std::snprintf(dst, dstLen, "%s", fallback);
    }
}

} // namespace

CadenceStrideModel::CadenceStrideModel(SDK::Interface::IFileSystem &fs,
                                       const char *outdoorLutPath,
                                       const char *deltaLutPath) :
    mFs(fs)
{
    copyPath(mOutdoorPath, sizeof(mOutdoorPath), outdoorLutPath,
             StrideLut::kDefaultPath);
    copyPath(mDeltaPath, sizeof(mDeltaPath), deltaLutPath, Config::kDeltaLutPath);
}

// --- Session start -----------------------------------------------------------

float CadenceStrideModel::clampHeight(float h)
{
    if (!std::isfinite(h) || h < Config::kHeightMinM || h > Config::kHeightMaxM) {
        return Config::kDefaultHeightM;
    }
    return h;
}

void CadenceStrideModel::startSession(float heightMeters)
{
    // 1. Read-only outdoor LUT (any failure → all-zero; the calibrator owns the
    //    file's integrity, so no .bak here).
    mOutdoor.loadFromFile(mFs, mOutdoorPath);

    // 2. Delta LUT (any failure → all-zero deltas).
    loadDeltaLut();

    // 3. Clamp the height for the demographic stride.
    mHeightM = clampHeight(heightMeters);

    // 4. Evaluate and FREEZE the phase for the whole session. Highest tier the
    //    data qualifies for: full (delta learns) > estimate (outdoor SL, delta
    //    frozen) > uncalibrated (demographic).
    if (mOutdoor.readyForPhase2()) {
        mPhase = Phase::OUTDOOR_CALIBRATED;
    } else if (mOutdoor.readyForOutdoorEstimate()) {
        mPhase = Phase::OUTDOOR_ESTIMATE;
    } else {
        mPhase = Phase::UNCALIBRATED;
    }
}

// --- Stride lookups ----------------------------------------------------------

float CadenceStrideModel::demographicStrideLengthM() const
{
    return clampf(Config::kDemographicK * mHeightM, Config::kStrideMinM,
                  Config::kStrideMaxM);
}

float CadenceStrideModel::outdoorStrideLengthM(float cadenceSpm) const
{
    // Find the nearest valid bin at or below c (largest valid centre <= c) and
    // at or above c (smallest valid centre >= c), skipping invalid bins.
    int loIdx = -1;
    int hiIdx = -1;
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        if (!mOutdoor.bin(i).isValid()) {
            continue;
        }
        const float centre = StrideLut::binCentreSpm(i);
        if (centre <= cadenceSpm) {
            loIdx = static_cast<int>(i);  // keep the largest valid centre <= c
        }
        if (centre >= cadenceSpm && hiIdx < 0) {
            hiIdx = static_cast<int>(i);  // first (smallest) valid centre >= c
        }
    }

    if (loIdx >= 0 && hiIdx >= 0) {
        if (loIdx == hiIdx) {
            return mOutdoor.bin(static_cast<size_t>(loIdx)).strideLengthM();
        }
        const float cLo = StrideLut::binCentreSpm(static_cast<size_t>(loIdx));
        const float cHi = StrideLut::binCentreSpm(static_cast<size_t>(hiIdx));
        const float slLo = mOutdoor.bin(static_cast<size_t>(loIdx)).strideLengthM();
        const float slHi = mOutdoor.bin(static_cast<size_t>(hiIdx)).strideLengthM();
        const float span = cHi - cLo;
        if (span <= 0.0f) {  // defensive; centres are distinct for loIdx != hiIdx
            return slLo;
        }
        return slLo + (slHi - slLo) * (cadenceSpm - cLo) / span;
    }
    if (loIdx >= 0) {
        // c above the highest valid bin → flat shelf.
        return mOutdoor.bin(static_cast<size_t>(loIdx)).strideLengthM();
    }
    if (hiIdx >= 0) {
        // c below the lowest valid bin → flat shelf.
        return mOutdoor.bin(static_cast<size_t>(hiIdx)).strideLengthM();
    }
    // No valid bin: cannot happen in phase 2 (gate guarantees >=8). Defensive
    // fallback to the demographic stride.
    return demographicStrideLengthM();
}

float CadenceStrideModel::deltaAt(float cadenceSpm) const
{
    // Dense interpolation across ALL 35 bin centres (no validity skipping —
    // every bin has a defined Δ, 0 where untouched), with flat shelves outside.
    const float c0 = StrideLut::binCentreSpm(0);
    const float cN = StrideLut::binCentreSpm(StrideLut::kBinCount - 1);
    if (cadenceSpm <= c0) {
        return mDelta[0];
    }
    if (cadenceSpm >= cN) {
        return mDelta[StrideLut::kBinCount - 1];
    }
    // Centres are evenly spaced by kBinWidthSpm, so position maps directly.
    const float pos = (cadenceSpm - c0) / Config::kBinWidthSpm;
    size_t i = static_cast<size_t>(std::floor(pos));
    if (i >= StrideLut::kBinCount - 1) {
        return mDelta[StrideLut::kBinCount - 1];
    }
    const float frac = pos - static_cast<float>(i);
    return mDelta[i] + frac * (mDelta[i + 1] - mDelta[i]);
}

float CadenceStrideModel::treadmillStrideLengthM(float cadenceSpm) const
{
    if (mPhase == Phase::UNCALIBRATED) {
        // Tier 1: constant demographic stride (already clamped); no LUT, no Δ.
        return demographicStrideLengthM();
    }
    // Tier 2 (estimate) and tier 3 (full) both use the outdoor SL(c). The
    // learned Δ(c) is added ONLY in the full tier; in the estimate tier the
    // delta is frozen at zero, but gating it explicitly keeps the estimate
    // tier independent of any delta file that might exist.
    float sl = outdoorStrideLengthM(cadenceSpm);
    if (mPhase == Phase::OUTDOOR_CALIBRATED) {
        sl += deltaAt(cadenceSpm);
    }
    return clampf(sl, Config::kStrideMinM, Config::kStrideMaxM);
}

// --- Post-run calibration ----------------------------------------------------

CadenceStrideModel::CalibrationResult CadenceStrideModel::applyPostRunCalibration(
    const float stepsPerBin[StrideLut::kBinCount], float D_estimated,
    float D_actual)
{
    // Total steps across the session.
    float sTotal = 0.0f;
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        const float s = stepsPerBin[i];
        if (std::isfinite(s) && s > 0.0f) {
            sTotal += s;
        }
    }

    // --- Sanity gates on D_actual (both must pass) --------------------------
    bool accepted = std::isfinite(D_actual) && D_actual > 0.0f;
    // Implied-stride gate.
    if (accepted) {
        if (sTotal <= 0.0f) {
            accepted = false;
        } else {
            const float impliedMeanStride = D_actual / (sTotal / 2.0f);
            if (impliedMeanStride < Config::kStrideMinM ||
                impliedMeanStride > Config::kStrideMaxM) {
                accepted = false;
            }
        }
    }
    // Ratio gate.
    if (accepted) {
        if (!(D_estimated > 0.0f)) {
            accepted = false;
        } else if (D_actual < Config::kDActualRatioMin * D_estimated ||
                   D_actual > Config::kDActualRatioMax * D_estimated) {
            accepted = false;
        }
    }

    CalibrationResult result;
    result.dActualAccepted = accepted;

    // Below the full tier (UNCALIBRATED or OUTDOOR_ESTIMATE): never touch the
    // delta LUT. Calibrate & Save still corrects the recorded distance (use
    // D_actual when accepted) — only the delta learning is withheld.
    if (mPhase != Phase::OUTDOOR_CALIBRATED) {
        result.distanceForFitM = accepted ? D_actual : D_estimated;
        result.deltaLutUpdated = false;
        return result;
    }

    // Phase 2, rejected: keep the estimate; leave the delta LUT untouched.
    if (!accepted) {
        result.distanceForFitM = D_estimated;
        result.deltaLutUpdated = false;
        return result;
    }

    // Phase 2, accepted: §5.3 weighted update over the used bins.
    const float deltaD = D_actual - D_estimated;
    float q = 0.0f;
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        const float s = stepsPerBin[i];
        if (std::isfinite(s) && s > 0.0f) {
            q += s * s;
        }
    }
    if (q > 0.0f) {  // accepted implies sTotal > 0 → at least one used bin
        for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
            const float s = stepsPerBin[i];
            if (!std::isfinite(s) || s <= 0.0f) {
                continue;  // bin not used this session — δ unchanged
            }
            const float dDelta = (2.0f * Config::kLearningRateEta * deltaD * s) / q;
            mDelta[i] += dDelta;

            // Per-bin clamp: keep the resulting bin stride plausible, then
            // back-solve the clamped δ. Use the bin's own SL when valid, else
            // the interpolated SL at its centre.
            const float slBin =
                mOutdoor.bin(i).isValid()
                    ? mOutdoor.bin(i).strideLengthM()
                    : outdoorStrideLengthM(StrideLut::binCentreSpm(i));
            const float clampedStride = clampf(slBin + mDelta[i],
                                               Config::kStrideMinM,
                                               Config::kStrideMaxM);
            mDelta[i] = clampedStride - slBin;
        }
    }

    saveDeltaLut();  // app logs a persist failure; learning already applied.

    result.distanceForFitM = D_actual;
    result.deltaLutUpdated = true;
    return result;
}

// --- Delta LUT persistence ---------------------------------------------------

void CadenceStrideModel::loadDeltaLut()
{
    // Start all-zero; only a fully valid file commits values.
    for (float &d : mDelta) {
        d = 0.0f;
    }

    std::unique_ptr<SDK::Interface::IFile> file = mFs.file(mDeltaPath);
    if (!file || !file->exist() || !file->open(false, false)) {
        return;  // missing / unopenable → zeros
    }

    const size_t size = file->size();
    if (size == 0 || size > StrideLut::kMaxStoreBytes) {
        file->close();
        return;  // empty / implausibly large → zeros
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
        return;
    }

    SDK::JsonStreamReader reader(buffer, size);
    bool ok = reader.validate();
    int32_t version = 0;
    if (ok) {
        ok = reader.get("version", version);
    }
    // version < ours → start all-zero (recoverable by recalibrating). version >
    // ours loads known fields, ignoring unknown keys (forward-compat).
    if (ok && version < Config::kDeltaLutVersion) {
        ok = false;
    }

    size_t arrayLen = 0;
    if (ok) {
        ok = reader.getArrayLength("deltas_m", arrayLen) &&
             arrayLen == StrideLut::kBinCount;  // wrong-length → zeros
    }

    if (ok) {
        float tmp[StrideLut::kBinCount] {};
        for (size_t i = 0; i < StrideLut::kBinCount && ok; ++i) {
            char query[32];
            std::snprintf(query, sizeof(query), "deltas_m[%d]",
                          static_cast<int>(i));
            float v = 0.0f;
            if (!reader.get(query, v) || !std::isfinite(v)) {
                ok = false;  // missing / non-finite entry → start all-zero
                break;
            }
            tmp[i] = v;
        }
        if (ok) {
            for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
                mDelta[i] = tmp[i];
            }
        }
    }

    delete[] buffer;
}

bool CadenceStrideModel::saveDeltaLut()
{
    // The delta file lives in the app's own root (no '/'), but mkdir the parent
    // defensively if a path with directories was injected.
    const char *slash = std::strrchr(mDeltaPath, '/');
    if (slash != nullptr) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
        std::snprintf(dir, sizeof(dir), "%.*s",
                      static_cast<int>(slash - mDeltaPath), mDeltaPath);
        if (!mFs.mkdir(dir)) {
            return false;
        }
    }

    std::unique_ptr<SDK::Interface::IFile> file = mFs.file(mDeltaPath);
    if (!file || !file->open(true, true)) {
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());
    writer.startMap();
    writer.add("version", static_cast<int32_t>(Config::kDeltaLutVersion));
    writer.startArray("deltas_m");
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        writer.add(mDelta[i]);
    }
    writer.endArray();
    writer.endMap();
    writer.flush();

    bool ok = !writer.isError();
    ok = file->flush() && ok;
    ok = file->close() && ok;
    return ok;
}

} // namespace SDK::Calibration
