/**
 ******************************************************************************
 * @file    TargetGauge.cpp
 * @brief   Pace and heart-rate gauges for workout step targets.
 ******************************************************************************
 */

#include "SDK/Workout/TargetGauge.hpp"

#include <cmath>

namespace SDK::Workout {

// --- ZoneHysteresis ------------------------------------------------------------

void ZoneHysteresis::reset()
{
    mZone    = Zone::Unknown;
    mPending = Zone::Unknown;
    mSeen    = 0;
}

Zone ZoneHysteresis::candidate(float v, float low, float high) const
{
    const float db = mDeadband;
    // Coming back In takes the deadband inside the band, capped at half the
    // band's width so that a narrow band can still be re-entered. A band of
    // zero width is re-entered on reaching it.
    const float half = (high - low) * 0.5f;
    const float in   = db < half ? db : half;
    switch (mZone) {
    case Zone::In:
        if (v < low - db)  return Zone::Below;
        if (v > high + db) return Zone::Above;
        return Zone::In;
    case Zone::Below:
        if (v > high + db) return Zone::Above;
        if (in > 0.0f ? v > low + in : v >= low) return Zone::In;
        return Zone::Below;
    case Zone::Above:
        if (v < low - db)  return Zone::Below;
        if (in > 0.0f ? v < high - in : v <= high) return Zone::In;
        return Zone::Above;
    default:
        // No zone yet: plain edges.
        if (v < low)  return Zone::Below;
        if (v > high) return Zone::Above;
        return Zone::In;
    }
}

Zone ZoneHysteresis::update(float value, float low, float high)
{
    const Zone c = candidate(value, low, high);
    if (c == mZone) {
        mPending = mZone;
        mSeen    = 0;
        return mZone;
    }
    if (c == mPending) {
        if (mSeen < UINT8_MAX) {
            ++mSeen;
        }
    } else {
        mPending = c;
        mSeen    = 1;
    }
    if (mSeen > mDwell) {
        mZone = c;
        mSeen = 0;
    }
    return mZone;
}

// --- SpeedGauge ----------------------------------------------------------------

namespace {

// Pace for a speed, in s/km. A standstill counts as very slow.
float paceSecPerKm(float mps)
{
    return mps > 0.05f ? 1000.0f / mps : 1.0e6f;
}

}  // namespace

void SpeedGauge::startStep(float lowMps, float highMps)
{
    mLow  = lowMps < highMps ? lowMps : highMps;
    mHigh = lowMps < highMps ? highMps : lowMps;
    mTargeted = true;
    restartStep();
}

void SpeedGauge::startStep()
{
    mTargeted = false;
    restartStep();
}

void SpeedGauge::restartStep()
{
    mSeconds = 0;
    mDist[0] = 0;  // the step's start
    // If the signal is lost as the step starts, the step's start distance is
    // from before the gap, like any second in it.
    setHadReading(0, mHadSignal);
    mStepAverage = 0.0f;
    mHadSignal = true;
    restartBlend();
}

void SpeedGauge::restartBlend()
{
    mBlendSec = 0;
    mHoldOff  = kHoldOffSec;
    mZone     = Zone::Unknown;
    mHyst.reset();
}

void SpeedGauge::setHadReading(uint32_t second, bool had)
{
    const uint32_t i = second % kHistory;
    const uint32_t bit = 1u << (i % 32);
    mHadReading[i / 32] = had ? (mHadReading[i / 32] | bit) : (mHadReading[i / 32] & ~bit);
}

bool SpeedGauge::hadReading(uint32_t second) const
{
    const uint32_t i = second % kHistory;
    return (mHadReading[i / 32] >> (i % 32)) & 1u;
}

void SpeedGauge::resume()
{
    restartBlend();
}

void SpeedGauge::tick(float liveMps, bool liveValid, uint32_t stepDistanceCm)
{
    // Distance is recorded every active second, signal or not, along with
    // whether there was a reading: once the signal returns the distance
    // catches up, and averages over the gap come out right.
    ++mSeconds;
    mDist[mSeconds % kHistory] = stepDistanceCm;
    setHadReading(mSeconds, liveValid);
    mStepAverage = static_cast<float>(stepDistanceCm) / 100.0f / static_cast<float>(mSeconds);

    if (!liveValid) {
        mHadSignal = false;
        mZone = Zone::NoSignal;  // the value shown stays the last good one
        return;
    }
    if (!mHadSignal) {
        mHadSignal = true;
        restartBlend();
    }

    // The latest second at least kWindowCm behind now; else the oldest kept
    // (the step's start while the step is younger than the history).
    // (The distance never goes backwards; if it does, count it as no progress.)
    const auto since = [&](uint32_t j) {
        const uint32_t then = mDist[j % kHistory];
        return stepDistanceCm > then ? stepDistanceCm - then : 0u;
    };
    const uint32_t reach = mSeconds < kHistory ? mSeconds : static_cast<uint32_t>(kHistory - 1);
    uint32_t from = mSeconds - reach;
    for (uint32_t age = 1; age <= reach; ++age) {
        if (since(mSeconds - age) >= kWindowCm) {
            from = mSeconds - age;
            break;
        }
    }
    // A second without a reading holds the distance from before the gap,
    // which the catch-up then lands on in one go. Start on the last second
    // before such a run, so that the gap's time is counted with its distance.
    while (!hadReading(from) && from > mSeconds - reach) {
        --from;
    }
    // If no second before the gap is kept (the gap began before the step, or
    // before the history), start on the first second after it instead: the
    // catch-up then falls before the window.
    while (!hadReading(from) && from + 1 < mSeconds) {
        ++from;
    }
    // from is at least a second back: reach is at least 1. On the second the
    // signal returns there is no such second, and no window to go on.
    const float window = hadReading(from)
        ? static_cast<float>(since(from)) / 100.0f / static_cast<float>(mSeconds - from)
        : liveMps;

    ++mBlendSec;
    const float w = std::exp(-static_cast<float>(mBlendSec) / kBlendTauSec);
    mValue = w * liveMps + (1.0f - w) * window;

    if (!mTargeted) {
        mZone = Zone::Unknown;
        return;
    }
    if (mHoldOff > 0) {
        --mHoldOff;
        mZone = Zone::Unknown;
        return;
    }
    // On pace a faster runner is lower, so the pace zones swap.
    const Zone onPace = mHyst.update(paceSecPerKm(mValue), paceSecPerKm(mHigh), paceSecPerKm(mLow));
    mZone = onPace == Zone::Below ? Zone::Above
          : onPace == Zone::Above ? Zone::Below
                                  : onPace;
}

// --- HeartRateGauge ------------------------------------------------------------

void HeartRateGauge::startStep(float lowBpm, float highBpm)
{
    mLow  = lowBpm < highBpm ? lowBpm : highBpm;
    mHigh = lowBpm < highBpm ? highBpm : lowBpm;
    mTargeted = true;
    mHadSignal = true;
    restart();
}

void HeartRateGauge::startStep()
{
    mTargeted = false;
    mHadSignal = true;
    restart();
}

void HeartRateGauge::resume()
{
    restart();
}

void HeartRateGauge::restart()
{
    mHoldOff = kHoldOffSec;
    mZone    = Zone::Unknown;
    mHyst.reset();
}

void HeartRateGauge::tick(float bpm, bool valid)
{
    if (!valid) {
        mHadSignal = false;
        mZone = Zone::NoSignal;
        return;
    }
    if (!mHadSignal) {
        mHadSignal = true;
        restart();
    }
    mValue = bpm;
    if (!mTargeted) {
        mZone = Zone::Unknown;
        return;
    }
    if (mHoldOff > 0) {
        --mHoldOff;
        mZone = Zone::Unknown;
        return;
    }
    mZone = mHyst.update(bpm, mLow, mHigh);
}

// --- Arc -----------------------------------------------------------------------

float arcFraction(float value, float low, float high, float margin)
{
    if (std::isnan(value)) {
        return 0.5f;
    }
    if (high < low) {
        const float t = low;
        low  = high;
        high = t;
    }
    if (margin <= 0.0f) {
        margin = 1.0e-6f;
    }
    constexpr float kThird = 1.0f / 3.0f;
    if (value <= low - margin) return 0.0f;
    if (value < low)           return (value - (low - margin)) / margin * kThird;
    if (value <= high) {
        return high > low ? kThird + (value - low) / (high - low) * kThird : 0.5f;
    }
    if (value < high + margin) return 2.0f * kThird + (value - high) / margin * kThird;
    return 1.0f;
}

}  // namespace SDK::Workout
