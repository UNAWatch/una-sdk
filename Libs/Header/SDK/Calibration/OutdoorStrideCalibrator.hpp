/**
 ******************************************************************************
 * @file    OutdoorStrideCalibrator.hpp
 * @brief   Background data-collection component that builds the outdoor
 *          stride-length LUT during outdoor Running sessions.
 *
 * Implements Docs/Treadmill/Outdoor-Data-Collection.md. A plain C++ object owned
 * by the Running app service task; ingestSample() is called inline at each 1 Hz
 * record-write point. No FreeRTOS task, no IPC. Persistence uses the SDK
 * JsonStreamReader / JsonStreamWriter over an injected IFileSystem.
 ******************************************************************************
 */

#ifndef __OUTDOOR_STRIDE_CALIBRATOR_HPP
#define __OUTDOOR_STRIDE_CALIBRATOR_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

namespace SDK::Calibration
{

/**
 * @brief Per-tick input sample assembled by the Running service (spec §2.3).
 *
 * A plain POD: each field is the latest latched value from its own sensor
 * stream at record-write time (the streams are not co-sampled).
 */
struct CalibratorSample {
    float gps_speed_ms          = 0.0f;   ///< Horizontal GPS speed, m/s.
    bool  gps_speed_valid       = false;  ///< False during acquisition / fix loss.
    bool  gps_fix_dead_reckoning = false; ///< True when the fix is estimated (§3.5).
    float cadence_spm           = 0.0f;   ///< Running cadence, steps/min.
    bool  cadence_valid         = false;  ///< Cadence estimator validity.
    float grade_pct             = 0.0f;   ///< Terrain grade, % (positive = uphill).
    bool  grade_valid           = false;  ///< False if grade cannot be computed.
    float delta_t_s             = 1.0f;   ///< Seconds since previous tick (~1.0).
};

/**
 * @brief One cadence bin's accumulated data (spec §5.2).
 */
struct StrideBin {
    float total_distance_m = 0.0f;  ///< Cumulative GPS distance from accepted samples.
    float total_steps      = 0.0f;  ///< Cumulative step count (float).
    float sample_count     = 0.0f;  ///< Accepted-sample count (erodes during aging).

    /// Bin is valid once enough steps are accumulated (spec §5.4).
    bool isValid() const { return total_steps >= Config::kBinValidMinSteps; }

    /// Has any accepted data been added.
    bool hasData() const { return total_steps > 0.0f; }

    /// Stride length lookup, metres: total_distance_m / (total_steps / 2).
    /// Returns 0 for an empty bin.
    float strideLengthM() const
    {
        return total_steps > 0.0f ? total_distance_m / (total_steps / 2.0f) : 0.0f;
    }
};

/**
 * @brief Outdoor stride calibrator (spec §1).
 *
 * Lifetime maps onto the Running session:
 *   - load()        at session start            (§6.3)
 *   - ingestSample() each 1 Hz tick while active (§4, §5)
 *   - pause()/resume() on run pause / resume     (§4.4)
 *   - finalise()    at session stop              (§6.4)
 */
class OutdoorStrideCalibrator {
public:
    /// Default calibration store path, relative to the app root (spec §6.1).
    static constexpr const char *kDefaultPath = "../SharedData/stride.json";

    /// Current persisted-schema version (spec §6.5).
    static constexpr int32_t kCurrentVersion = 1;

    /// Maximum accepted store-file size. The store holds 35 small bins (a few
    /// KB); larger files are treated as corrupt to bound transient heap
    /// allocation on embedded targets.
    static constexpr size_t kMaxStoreBytes = 16u * 1024u;

    /**
     * @brief Construct over a filesystem.
     * @param fs   Filesystem used for load / save (the app's guarded FS).
     * @param path Calibration store path; defaults to kDefaultPath.
     */
    explicit OutdoorStrideCalibrator(SDK::Interface::IFileSystem &fs,
                                     const char *path = kDefaultPath);

    ~OutdoorStrideCalibrator();

    // --- Debug trace (spec §9.2) ---------------------------------------------

    /**
     * @brief Enable a per-tick CSV trace (one row per ingestSample: raw sample,
     *        each gate's pass/fail, steady-state counter, accept/discard).
     *
     * Opens @p path for writing (truncating any existing file) and writes the
     * header. Off by default; gated by an app config flag. A failed open
     * silently leaves tracing disabled. See spec §9.2.
     * @param path Trace file path (e.g. "../SharedData/stride_trace.csv").
     */
    void enableTrace(const char *path);

    // --- Session lifecycle ---------------------------------------------------

    /// Load the store into memory (spec §6.3). Safe to call once at start.
    void load();

    /// Gate, aggregate, or discard one tick (spec §4, §5).
    void ingestSample(const CalibratorSample &sample);

    /// Stop ingesting and reset the steady-state counter (spec §4.4).
    void pause();

    /// Resume ingesting; the counter restarts from zero (spec §4.4).
    void resume();

    /**
     * @brief Persist the store if >= 1 sample was accepted this session (§6.4).
     * @return true if the file was written, false if skipped (nothing accepted)
     *         or on write error.
     */
    bool finalise();

    // --- Accessors -----------------------------------------------------------

    /// Number of bins (always Config::kBinCount).
    static constexpr size_t binCount() { return Config::kBinCount; }

    /// Read one bin (no bounds check; index must be < binCount()).
    const StrideBin &bin(size_t index) const { return mBins[index]; }

    /// Centre cadence (SPM) of a bin: 82, 86, ..., 218.
    static float binCentreSpm(size_t index);

    /// Bin index for a cadence value, clamped to [0, kBinCount-1] (spec §5.1).
    static size_t binIndexForCadence(float cadenceSpm);

    /// Accepted-sample count for the current session (drives the §6.4 write gate).
    size_t acceptedThisSession() const { return mSessionAccepted; }

    /// Current steady-state counter, seconds (diagnostic / test).
    float steadySeconds() const { return mSteadySeconds; }

    // --- Phase-2 gate read path (spec §7) ------------------------------------

    /// Count of valid bins (spec §5.4).
    size_t validBinCount() const;

    /// Total accumulated calibration distance across all bins, metres.
    float totalCalibrationDistanceM() const;

    /// True when both phase-2 conditions hold (spec §7).
    bool readyForPhase2() const;

private:
    /// Reset the steady-state run: counter to zero, reference values cleared.
    void resetSteadyState();

    /// Add an accepted sample to its bin, applying distance-cap aging (§5.3).
    void accumulate(const CalibratorSample &sample);

    /// Zero all bins (fresh start).
    void clearBins();

    /// Parse a previously read JSON buffer into mBins. @return false on failure.
    bool parseBuffer(const char *data, size_t len);

    /// Back up the current store file to "<path>.bak".
    void backupCorruptFile();

    /// Per-tick gate outcomes captured for the debug trace (spec §9.2).
    struct GateTrace {
        bool gpsValid     = false;
        bool fixOk        = false;
        bool speedBounds  = false;
        bool cadValid     = false;
        bool cadBounds    = false;
        bool gradeValid   = false;
        bool gradeBounds  = false;
        bool steadyEval   = false;  ///< Were the steady-state gates evaluated this tick.
        bool speedSteady  = false;
        bool cadSteady    = false;
    };

    /// Append one CSV row to the trace file (no-op if tracing disabled).
    void writeTraceRow(const CalibratorSample &sample, const GateTrace &g,
                       const char *verdict);

    /// Close the trace file if open.
    void closeTrace();

    SDK::Interface::IFileSystem &mFs;
    /// Copy of the store path (the calibrator outlives the ctor argument).
    char mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};

    StrideBin mBins[Config::kBinCount] {};

    // Steady-state machine (spec §4).
    float  mSteadySeconds = 0.0f;   ///< Accumulated qualifying seconds.
    float  mSpeedRef      = 0.0f;   ///< Held speed reference for the band gate.
    float  mCadenceRef    = 0.0f;   ///< Held cadence reference for the band gate.
    bool   mRefsSet       = false;  ///< Reference values latched for this run.
    bool   mPaused        = false;  ///< Ingestion halted between pause/resume.

    size_t mSessionAccepted = 0;    ///< Accepted samples this session (§6.4 gate).

    // Debug trace (spec §9.2); inactive unless enableTrace() succeeds.
    std::unique_ptr<SDK::Interface::IFile> mTraceFile;
    uint32_t mTraceRow = 0;
};

} // namespace SDK::Calibration

#endif /* __OUTDOOR_STRIDE_CALIBRATOR_HPP */
