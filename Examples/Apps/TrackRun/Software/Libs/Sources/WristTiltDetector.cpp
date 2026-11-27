/*******************************************************************************
 * @file   WristTiltDetector.cpp
 * @date   22-04-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief  Wrist-tilt gesture detector implementation.
 ******************************************************************************/

#include "WristTiltDetector.hpp"

#include <cstring>
#include <cmath>

 /*******************************************************************************
  * Construction / lifecycle
  ******************************************************************************/

WristTiltDetector::WristTiltDetector(const Config& cfg) noexcept
    : mCfg(cfg)
{
    const float sr = cfg.sampleRateHz;

    mMotionCapacity = static_cast<uint16_t>(cfg.motionWindowS * sr + 0.5f);
    if (mMotionCapacity > MAX_MOTION_WIN) {
        mMotionCapacity = MAX_MOTION_WIN;
    }
    if (mMotionCapacity == 0u) {
        mMotionCapacity = 1u;
    }

    mHoldSamples = static_cast<uint32_t>(cfg.holdDurationS * sr + 0.5f);
    mCooldownSamples = static_cast<uint32_t>(cfg.cooldownDurationS * sr + 0.5f);

    /* Precompute tan(|pitchThresholdDeg|) for the trig-free pitch test. */
    constexpr float kDegToRad = 0.0174532925199433f;
    mNegPitchTan = std::tan(-cfg.pitchThresholdDeg * kDegToRad);

    reset();
}

void WristTiltDetector::setListener(IListener* listener) noexcept
{
    mListener = listener;
}

void WristTiltDetector::reset() noexcept
{
    mMotionHead = 0u;
    mMotionFilled = 0u;
    mDeltaSum = 0.0f;
    mPrevAy = 0;
    mHasPrevAy = false;

    mGravY = 0.0f;
    mGravZ = 0.0f;

    mTiltState = TiltState::IDLE;
    mStateDeadline = 0u;
    mSampleCount = 0u;

    mMotionClass = MotionClass::STATIONARY;
    mLastAvgSwing = 0.0f;
    mTotalEvents = 0u;

    std::memset(mDeltaBuf, 0, sizeof(mDeltaBuf));
}

void WristTiltDetector::setConfig(const Config& cfg) noexcept
{
    mCfg = cfg;

    const float sr = cfg.sampleRateHz;

    mMotionCapacity = static_cast<uint16_t>(cfg.motionWindowS * sr + 0.5f);
    if (mMotionCapacity > MAX_MOTION_WIN) {
        mMotionCapacity = MAX_MOTION_WIN;
    }
    if (mMotionCapacity == 0u) {
        mMotionCapacity = 1u;
    }

    mHoldSamples = static_cast<uint32_t>(cfg.holdDurationS * sr + 0.5f);
    mCooldownSamples = static_cast<uint32_t>(cfg.cooldownDurationS * sr + 0.5f);

    /* Precompute tan(|pitchThresholdDeg|) for the trig-free pitch test. */
    constexpr float kDegToRad = 0.0174532925199433f;
    mNegPitchTan = std::tan(-cfg.pitchThresholdDeg * kDegToRad);

    reset();
}

/*******************************************************************************
 * Main processing
 ******************************************************************************/

bool WristTiltDetector::addBatch(const TiltImuSample* samples,
    size_t               count) noexcept
{
    if (samples == nullptr || count == 0u) {
        return false;
    }

    bool anyTriggered = false;

    for (size_t i = 0u; i < count; ++i) {
        if (processSample(samples[i])) {
            anyTriggered = true;
        }
    }

    return anyTriggered;
}

/*******************************************************************************
 * Private helpers
 ******************************************************************************/

void WristTiltDetector::pushMotionSample(int16_t ay, int16_t prevAy) noexcept
{
    const float delta = static_cast<float>(
        (ay >= prevAy) ? (ay - prevAy) : (prevAy - ay));

    /* Evict oldest sample from ring if full. */
    if (mMotionFilled >= mMotionCapacity) {
        mDeltaSum -= mDeltaBuf[mMotionHead];
    } else {
        ++mMotionFilled;
    }

    mDeltaBuf[mMotionHead] = delta;
    mDeltaSum += delta;

    mMotionHead = static_cast<uint16_t>((mMotionHead + 1u) % mMotionCapacity);
}

float WristTiltDetector::computeAvgSwing() const noexcept
{
    if (mMotionFilled < 1u) {
        return 0.0f;
    }
    /* mMotionFilled is the count of stored deltas. */
    return mDeltaSum / static_cast<float>(mMotionFilled);
}

void WristTiltDetector::updateMotionClass(float    avgSwing,
    uint32_t tsMs) noexcept
{
    MotionClass next = mMotionClass;

    if (avgSwing > mCfg.avgSwingRunning) {
        next = MotionClass::RUNNING;
    } else if (avgSwing > mCfg.avgSwingWalking) {
        next = MotionClass::WALKING;
    } else if (avgSwing < mCfg.avgSwingStationary) {
        next = MotionClass::STATIONARY;
    }
    /* Else: stay in current class (hysteresis band). */

    if (next != mMotionClass) {
        mMotionClass = next;
        if (mListener != nullptr) {
            mListener->onMotionClassChange(mMotionClass, tsMs);
        }
    }
}

void WristTiltDetector::updateGravity(int16_t ay, int16_t az) noexcept
{
    /* Per-axis low-pass filter isolating the gravity vector. */
    const float alpha = mCfg.gravityAlpha;
    mGravY = alpha * mGravY + (1.0f - alpha) * static_cast<float>(ay);
    mGravZ = alpha * mGravZ + (1.0f - alpha) * static_cast<float>(az);
}

bool WristTiltDetector::isPitchAboveThreshold() const noexcept
{
    /* pitch = atan2(grav_y, grav_z) > pitchThresholdDeg, without trig.
       The threshold is non-positive, so the whole grav_y >= 0 half-plane
       (pitch in [0, 180] deg) clears it.  For grav_y < 0 the pitch is
       negative and exceeds the threshold only in the grav_z > 0 quadrant
       and within the wedge |pitch| < |threshold|, i.e.
           -grav_y < grav_z * tan(|threshold|). */
    if (mGravY >= 0.0f) {
        return true;
    }
    return (mGravZ > 0.0f) && (mGravZ * mNegPitchTan > -mGravY);
}

bool WristTiltDetector::processSample(const TiltImuSample& sample) noexcept
{
    const uint32_t now = mSampleCount;
    ++mSampleCount;

    /* --- 1. Update motion window ------------------------------------------ */
    if (mHasPrevAy) {
        pushMotionSample(sample.ayLsb, mPrevAy);
    } else {
        mHasPrevAy = true;
    }
    mPrevAy = sample.ayLsb;

    const float avgSwing = computeAvgSwing();
    mLastAvgSwing = avgSwing;
    updateMotionClass(avgSwing, sample.timestampMs);

    /* --- 2. Gravity filter (updated every sample, all motion classes) ----- */
    updateGravity(sample.ayLsb, sample.azLsb);

    /* --- 3. Tilt state machine -------------------------------------------- */
    /* The machine stays alive in every motion class so a hold or cooldown
       in progress completes its timed transitions.  Only the IDLE -> ACTIVE
       trigger is gated to RUNNING: in STATIONARY / WALKING the physical
       WRIST_MOTION sensor owns the gesture. */
    const int32_t gxMag = (sample.gxLsb >= 0)
        ? static_cast<int32_t>(sample.gxLsb)
        : -static_cast<int32_t>(sample.gxLsb);
    const bool gxAbove = (gxMag > mCfg.gxThreshRunning);
    const bool pitchAbove = isPitchAboveThreshold();
    const bool canTrigger = (mMotionClass == MotionClass::RUNNING)
                            && (gxAbove || pitchAbove);

    bool fired = false;

    switch (mTiltState) {
    case TiltState::IDLE:
        if (canTrigger) {
            mTiltState = TiltState::ACTIVE;
            mStateDeadline = now + mHoldSamples;
            ++mTotalEvents;
            fired = true;
            if (mListener != nullptr) {
                mListener->onWristTilt(sample.timestampMs);
            }
        }
        break;

    case TiltState::ACTIVE:
        if (now >= mStateDeadline) {
            mTiltState = TiltState::COOLDOWN;
            mStateDeadline = now + mCooldownSamples;
        }
        break;

    case TiltState::COOLDOWN:
        if (now >= mStateDeadline) {
            mTiltState = TiltState::IDLE;
        }
        break;
    }

    return fired;
}
