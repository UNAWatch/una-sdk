/**
 ******************************************************************************
 * @file    StrideLut.hpp
 * @brief   Shared 35-bin cadence→stride lookup table + stride.json loader.
 *
 * Extracted from OutdoorStrideCalibrator so the bin storage and the
 * stride.json JSON parse are shared by both the calibrator (write side) and
 * the cadence/stride model (read side). The calibrator composes a StrideLut
 * and keeps its own write/backup orchestration; the model loads one read-only.
 *
 * Holds no acceptance state machine — purely the bin grid, persistence parse,
 * and the phase-2 readiness predicate.
 ******************************************************************************
 */

#ifndef __STRIDE_LUT_HPP
#define __STRIDE_LUT_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

namespace SDK::Calibration
{

/**
 * @brief One cadence bin's accumulated data.
 *
 * Moved verbatim from OutdoorStrideCalibrator so both the calibrator and the
 * model share one definition.
 */
struct StrideBin {
    float total_distance_m = 0.0f;  ///< Cumulative GPS distance from accepted samples.
    float total_steps      = 0.0f;  ///< Cumulative step count (float).
    float sample_count     = 0.0f;  ///< Accepted-sample count (erodes during aging).

    /// Bin is valid once enough steps are accumulated.
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
 * @brief Shared cadence→stride lookup table (35 bins) with stride.json I/O.
 *
 * The bin grid is fixed: centres 82, 86, ..., 218 SPM. The class owns only the
 * bin array and parse logic; load policy (backup-on-corrupt) belongs to the
 * write owner (the calibrator). The read owner (the model) uses
 * loadFromFile(), which never writes a .bak.
 */
class StrideLut {
public:
    /// Number of cadence bins.
    static constexpr size_t kBinCount = Config::kBinCount;  // 35

    /// Default store path, relative to the app root.
    static constexpr const char *kDefaultPath = "../SharedData/stride.json";

    /// Persisted-schema version understood by the parser.
    static constexpr int32_t kStoreVersion = 1;

    /// Maximum accepted store-file size. The store holds 35 small bins (a few
    /// KB); larger files are treated as corrupt to bound transient heap
    /// allocation on embedded targets.
    static constexpr size_t kMaxStoreBytes = 16u * 1024u;

    // --- Bin layout helpers -------------------------------------------------

    /// Centre cadence (SPM) of a bin: 82, 86, ..., 218.
    static float binCentreSpm(size_t index);

    /// Bin index for a cadence value, clamped to [0, kBinCount-1].
    static size_t binIndexForCadence(float cadenceSpm);

    // --- Persistence --------------------------------------------------------

    /**
     * @brief Read-only load: parse stride.json into the bins.
     *
     * Starts by clearing the LUT. On any failure (missing / unopenable / empty
     * / oversized / unreadable / malformed) the LUT is left all-zero and false
     * is returned — no .bak is written (the calibrator owns that file's
     * integrity). A syntactically valid but older-version file loads as empty
     * and still returns true.
     *
     * @param fs   Filesystem used for the read.
     * @param path Store path; defaults to kDefaultPath.
     * @return true if a valid file was parsed (even if it contained no bins).
     */
    bool loadFromFile(SDK::Interface::IFileSystem &fs,
                      const char *path = kDefaultPath);

    /**
     * @brief Parse a previously read JSON buffer into the bins.
     *
     * Shared by the read-only loader above and the calibrator's write-owner
     * load(), so there is exactly one parser. Does NOT clear the bins first;
     * the caller clears before calling.
     *
     * @return false on malformed JSON (caller may back up); true otherwise
     *         (including an older-version file, which loads as empty).
     */
    bool parseBuffer(const char *data, size_t len);

    /// Zero all bins.
    void clear();

    // --- Accessors ----------------------------------------------------------

    const StrideBin &bin(size_t index) const { return mBins[index]; }
    StrideBin       &bin(size_t index)       { return mBins[index]; }

    /// Count of valid bins.
    size_t validBinCount() const;

    /// Total accumulated calibration distance across all bins, metres.
    float totalCalibrationDistanceM() const;

    /// True when both phase-2 conditions hold (>=8 valid bins and >=5000 m).
    /// At this point the delta LUT may begin learning.
    bool readyForPhase2() const;

    /// True once there is enough outdoor data to use the LUT for live
    /// estimation (>= kOutdoorLutMinValidBinsEstimate valid bins) while the
    /// delta LUT stays frozen. Weaker than (and implied by) readyForPhase2().
    bool readyForOutdoorEstimate() const;

private:
    StrideBin mBins[kBinCount] {};
};

} // namespace SDK::Calibration

#endif /* __STRIDE_LUT_HPP */
