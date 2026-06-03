/**
 ******************************************************************************
 * @file    TreadmillSpeedEstimator.cpp
 * @brief   Implementation of the live treadmill speed/distance estimator.
 ******************************************************************************
 */

#include "SDK/Calibration/TreadmillSpeedEstimator.hpp"

#include <cmath>

#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"

namespace SDK::Calibration
{

namespace
{

/// Clamp dt into [0, hi]. Non-finite or negative dt collapses to 0 (no
/// integration this tick); an over-long gap is bounded to hi.
float clampDt(float dt, float hi)
{
    if (!(dt > 0.0f)) {  // dt <= 0 or NaN
        return 0.0f;
    }
    return dt > hi ? hi : dt;
}

} // namespace

TreadmillSpeedEstimator::TreadmillSpeedEstimator(const CadenceStrideModel &model) :
    mModel(model)
{
}

void TreadmillSpeedEstimator::startSession()
{
    mDistanceM = 0.0;
    for (float &s : mSteps) {
        s = 0.0f;
    }
    mSpeedMps     = 0.0f;
    mSpeedValid   = false;
    mHeldSpeedMps = 0.0f;
    mHeldCadence  = 0.0f;
    mHoldElapsedS = 0.0f;
    mHaveHeld     = false;
    mPaused       = false;
}

void TreadmillSpeedEstimator::tick(float cadenceSpm, bool cadenceValid, float dt)
{
    if (mPaused) {
        // Pause halts integration entirely and overrides any hold window.
        mSpeedMps   = 0.0f;
        mSpeedValid = false;
        return;
    }

    dt = clampDt(dt, Config::kMaxTickGapS);

    if (cadenceValid) {
        const float stride = mModel.treadmillStrideLengthM(cadenceSpm);
        const float speed  = (cadenceSpm / 120.0f) * stride;

        mDistanceM += static_cast<double>(speed) * static_cast<double>(dt);
        mSteps[StrideLut::binIndexForCadence(cadenceSpm)] +=
            cadenceSpm * dt / 60.0f;

        // Latch the held value and reset the hold window.
        mHeldSpeedMps = speed;
        mHeldCadence  = cadenceSpm;
        mHoldElapsedS = 0.0f;
        mHaveHeld     = true;

        mSpeedMps   = speed;
        mSpeedValid = true;
        return;
    }

    // Cadence invalid: hold the last valid speed for up to kCadenceHoldSeconds,
    // continuing to integrate distance and S_i at the held cadence/speed.
    mHoldElapsedS += dt;
    if (mHaveHeld && mHoldElapsedS <= Config::kCadenceHoldSeconds) {
        mDistanceM += static_cast<double>(mHeldSpeedMps) * static_cast<double>(dt);
        mSteps[StrideLut::binIndexForCadence(mHeldCadence)] +=
            mHeldCadence * dt / 60.0f;
        mSpeedMps   = mHeldSpeedMps;
        mSpeedValid = true;
    } else {
        // Hold expired or never had a valid sample: stop integrating.
        mSpeedMps   = 0.0f;
        mSpeedValid = false;
    }
}

void TreadmillSpeedEstimator::pause()
{
    mPaused     = true;
    mSpeedMps   = 0.0f;
    mSpeedValid = false;
}

void TreadmillSpeedEstimator::resume()
{
    mPaused = false;
    // Clear the hold so a fresh valid cadence re-latches the held value.
    mHaveHeld     = false;
    mHoldElapsedS = 0.0f;
    mHeldSpeedMps = 0.0f;
    mHeldCadence  = 0.0f;
    mSpeedMps     = 0.0f;
    mSpeedValid   = false;
}

} // namespace SDK::Calibration
