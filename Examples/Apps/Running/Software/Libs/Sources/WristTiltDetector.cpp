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
    if (mMotionFilled < 2u) {
        return 0.0f;
    }
    /* mMotionFilled - 1 because we store N deltas for N+1 samples. */
    return mDeltaSum / static_cast<float>(mMotionFilled - 1u);
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

int32_t WristTiltDetector::dynamicGxThreshold() const noexcept
{
    switch (mMotionClass) {
    case MotionClass::RUNNING:    return mCfg.gxThreshRunning;
    case MotionClass::WALKING:    return mCfg.gxThreshWalking;
    case MotionClass::STATIONARY: return mCfg.gxThreshStationary;
    default:                      return mCfg.gxThreshStationary;
    }
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

    /* --- 2. Tilt state machine -------------------------------------------- */
    const int32_t gxMag = (sample.gxLsb >= 0)
        ? static_cast<int32_t>(sample.gxLsb)
        : -static_cast<int32_t>(sample.gxLsb);
    const int32_t gxThresh = dynamicGxThreshold();
    const bool    gxAbove = (gxMag > gxThresh);

    bool fired = false;

    switch (mTiltState) {
    case TiltState::IDLE:
        if (gxAbove) {
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