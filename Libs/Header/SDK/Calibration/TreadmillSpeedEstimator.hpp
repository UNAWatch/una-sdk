/**
 ******************************************************************************
 * @file    TreadmillSpeedEstimator.hpp
 * @brief   Live 1 Hz speed/distance integration for the Treadmill activity.
 *
 * Owns the cumulative distance, the per-bin step histogram S_i, and the
 * cadence hold-forward state. Runs in the Treadmill app's Service process as a
 * plain C++ object — no FreeRTOS task, no IPC, no direct sensor reads (the
 * Service pushes cadence in via tick()).
 ******************************************************************************
 */

#ifndef __TREADMILL_SPEED_ESTIMATOR_HPP
#define __TREADMILL_SPEED_ESTIMATOR_HPP

#include "SDK/Calibration/CadenceStrideModel.hpp"
#include "SDK/Calibration/StrideLut.hpp"

namespace SDK::Calibration
{

/**
 * @brief Live treadmill speed/distance estimator.
 */
class TreadmillSpeedEstimator {
public:
    /// @param model Cadence/stride model (must outlive the estimator).
    explicit TreadmillSpeedEstimator(const CadenceStrideModel &model);

    /// Reset distance, S_i, hold state and pause flag for a new session.
    void startSession();

    /**
     * @brief One Service tick.
     * @param cadenceSpm   Cadence, steps/min (from the kernel RUNNING_CADENCE).
     * @param cadenceValid Cadence estimator validity.
     * @param dt           Seconds since the previous tick (~1.0).
     */
    void tick(float cadenceSpm, bool cadenceValid, float dt);

    /// Halt integration immediately; speed → 0 (overrides any hold).
    void pause();

    /// Resume integration on subsequent ticks; clears hold state.
    void resume();

    /// Current speed, m/s (0 when paused or invalid+hold expired).
    float speedMps() const { return mSpeedValid ? mSpeedMps : 0.0f; }

    /// False → the app shows "—" for speed.
    bool speedValid() const { return mSpeedValid; }

    /// Cumulative distance, metres (pre-correction = D_estimated).
    float distanceM() const { return static_cast<float>(mDistanceM); }

    /// Per-bin step histogram S_i (length StrideLut::kBinCount).
    const float *stepHistogram() const { return mSteps; }

    /// Convenience alias for the cumulative distance (estimator output).
    float estimatedDistanceM() const { return distanceM(); }

private:
    const CadenceStrideModel &mModel;

    double mDistanceM = 0.0;                 ///< Cumulative distance (double for precision).
    float  mSteps[StrideLut::kBinCount] {};  ///< Per-bin step accumulation S_i.

    float  mSpeedMps     = 0.0f;             ///< Last computed speed.
    bool   mSpeedValid   = false;            ///< Speed currently reportable.

    float  mHeldSpeedMps = 0.0f;             ///< Last valid speed (for hold-forward).
    float  mHeldCadence  = 0.0f;             ///< Last valid cadence.
    float  mHoldElapsedS = 0.0f;             ///< Time since cadence went invalid.
    bool   mHaveHeld     = false;            ///< A valid sample has been latched.
    bool   mPaused       = false;            ///< Integration halted between pause/resume.
};

} // namespace SDK::Calibration

#endif /* __TREADMILL_SPEED_ESTIMATOR_HPP */
