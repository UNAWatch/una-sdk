/**
 ******************************************************************************
 * @file    CadenceStrideModel.hpp
 * @brief   Read-side cadence→stride model for the Treadmill activity.
 *
 * Owns the read-only outdoor stride LUT and the Treadmill-private delta LUT.
 * Resolves the calibration phase once at session start (frozen thereafter),
 * provides the per-tick stride lookup the live estimator drives, and applies
 * the post-run weighted delta calibration.
 *
 * A plain C++ object owned by the Treadmill app's Service task — no IPC, no
 * FreeRTOS task, no direct sensor reads. Filesystem access is constructor-
 * injected so host tests use a fake.
 ******************************************************************************
 */

#ifndef __CADENCE_STRIDE_MODEL_HPP
#define __CADENCE_STRIDE_MODEL_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Calibration/CadenceStrideModelConfig.hpp"
#include "SDK/Calibration/StrideLut.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

namespace SDK::Calibration
{

/**
 * @brief Calibration phase, frozen at session start.
 *
 * Numeric values match the parent spec (1 = uncalibrated demographic fallback,
 * 2 = outdoor-calibrated LUT in use).
 */
enum class Phase : uint8_t {
    UNCALIBRATED       = 1,  ///< Outdoor LUT not yet ready — demographic stride.
    OUTDOOR_CALIBRATED = 2,  ///< Outdoor LUT ready — SL(c) + Δ(c).
};

/**
 * @brief Cadence→stride model consumed by TreadmillSpeedEstimator.
 */
class CadenceStrideModel {
public:
    /**
     * @brief Construct over a filesystem.
     * @param fs            Filesystem for LUT load / delta save.
     * @param outdoorLutPath Outdoor (read-only) LUT path; defaults to the
     *                       shared stride.json.
     * @param deltaLutPath   Treadmill-app-local delta LUT filename.
     */
    explicit CadenceStrideModel(
        SDK::Interface::IFileSystem &fs,
        const char *outdoorLutPath = StrideLut::kDefaultPath,
        const char *deltaLutPath   = Config::kDeltaLutPath);

    /**
     * @brief Called once at treadmill session start.
     *
     * Loads the outdoor LUT (read-only) and the delta LUT, clamps the height,
     * and evaluates + freezes the phase for the whole session.
     *
     * @param heightMeters Profile height in metres (heightCm / 100). Clamped to
     *                     a plausible window, falling back to a default.
     */
    void startSession(float heightMeters);

    /// Frozen phase resolved at startSession().
    Phase phase() const { return mPhase; }

    /// Convenience: true iff phase() == OUTDOOR_CALIBRATED.
    bool outdoorLutReady() const { return mPhase == Phase::OUTDOOR_CALIBRATED; }

    /// Clamped height used for the demographic stride, metres (diagnostic/test).
    float heightM() const { return mHeightM; }

    /**
     * @brief Live per-tick stride length, metres (left-to-left).
     *
     * Phase 1: clamped demographic stride (constant vs cadence, no Δ).
     * Phase 2: clamp(SL(c) + Δ(c)). Result is clamped to [kStrideMinM,
     * kStrideMaxM]. cadenceSpm is assumed already validity-checked by the caller.
     */
    float treadmillStrideLengthM(float cadenceSpm) const;

    /// Result of a post-run calibration pass.
    struct CalibrationResult {
        float distanceForFitM = 0.0f;   ///< Distance to record (D_actual if accepted, else D_estimated).
        bool  deltaLutUpdated = false;  ///< True only in phase 2 with an accepted D_actual.
        bool  dActualAccepted = false;  ///< False if D_actual rejected by the sanity gates.
    };

    /**
     * @brief Post-run weighted delta update (called after stop).
     *
     * Phase 1: never touches the delta LUT; returns the FIT/summary distance.
     * Phase 2: applies the sanity gates, the weighted delta update, the per-bin
     * stride clamp, then persists the delta LUT.
     *
     * @param stepsPerBin S_i accumulated this session (length StrideLut::kBinCount).
     * @param D_estimated Estimator cumulative distance (pre-correction), metres.
     * @param D_actual    Treadmill console distance in metres (app pre-converts).
     */
    CalibrationResult applyPostRunCalibration(
        const float stepsPerBin[StrideLut::kBinCount],
        float D_estimated, float D_actual);

    // --- Lower-level lookups (exposed for testing / app diagnostics) --------

    /// Phase-2 outdoor SL(c): valid-bin interpolation with flat shelves.
    float outdoorStrideLengthM(float cadenceSpm) const;

    /// Δ(c): dense interpolation across all 35 bin centres (flat shelves).
    float deltaAt(float cadenceSpm) const;

    /// Demographic SL = clamp(kDemographicK * height).
    float demographicStrideLengthM() const;

private:
    /// Clamp a height to the plausible window, falling back to the default.
    static float clampHeight(float h);

    /// Load the delta LUT from mDeltaPath (all-zero on any failure).
    void loadDeltaLut();

    /// Persist the delta LUT to mDeltaPath. @return false on write error.
    bool saveDeltaLut();

    SDK::Interface::IFileSystem &mFs;
    char  mOutdoorPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char  mDeltaPath[SDK::Interface::IFileSystem::skMaxPathLen] {};

    StrideLut mOutdoor;                          ///< Read-only outdoor LUT.
    float     mDelta[StrideLut::kBinCount] {};   ///< Dense delta, metres, 0 = untouched.
    Phase     mPhase   = Phase::UNCALIBRATED;    ///< Frozen at startSession().
    float     mHeightM = Config::kDefaultHeightM;
};

} // namespace SDK::Calibration

#endif /* __CADENCE_STRIDE_MODEL_HPP */
