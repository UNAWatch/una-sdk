/**
 ******************************************************************************
 * @file    TargetGauge.hpp
 * @brief   How a runner is doing against a workout step's pace or heart-rate
 *          target: a value to show, and a zone (below, in or above the band).
 *
 * SpeedGauge, for speed (pace) targets, ticks once per active second with the
 * live speed and the step's distance so far. Its value blends two inputs:
 *   - the live speed, which responds quickly;
 *   - the average speed over the step's last kWindowCm of distance, or over
 *     the whole step until it is that long, which is steadier.
 * At the start of a step only the live speed exists, so the gauge starts
 * there and moves to the window average with time constant kBlendTauSec:
 *
 *     w     = exp(-t / kBlendTauSec)        t = seconds since (re)starting
 *     value = w * live + (1 - w) * window
 *
 * The zone is decided on pace with a deadband and a dwell (ZoneHysteresis), so
 * it does not flicker at a band edge. The zone is Unknown for the first
 * kHoldOffSec ticks with a reading after the step starts, the run resumes or
 * the signal returns, and the dwell follows, so the first zone comes on tick
 * kHoldOffSec + kDwellTicks + 1 (the 8th).
 *
 * HeartRateGauge does the same for heart-rate targets on the displayed heart
 * rate, without blending.
 *
 * Neither allocates. Tick only while the run is active: a paused second must
 * not be passed in, and the step distance must not grow while paused.
 * A SpeedGauge is about 1 KB: keep it as a member, not on a task's stack.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_TARGET_GAUGE_HPP
#define SDK_WORKOUT_TARGET_GAUGE_HPP

#include <cstddef>
#include <cstdint>

namespace SDK::Workout {

/// Where a value sits against a target band.
enum class Zone : uint8_t {
    Unknown,   ///< Too early to tell (hold-off) or no target.
    NoSignal,  ///< No current reading: the value shown is the last good one,
               ///< which may be from the step before.
    Below,     ///< Too slow, or heart rate too low.
    In,        ///< In the band.
    Above,     ///< Too fast, or heart rate too high.
};

/**
 * Below / In / Above for a value against a band [low, high], with a deadband
 * and a dwell:
 *   - leaving In takes more than @p deadband past an edge;
 *   - coming back In takes more than @p deadband inside the band, or more
 *     than half the band's width when that is less (reaching the band when
 *     it has no width);
 *   - a new zone must be seen on more than @p dwell consecutive updates
 *     before it is taken.
 * The first zone after reset() needs no deadband, only the dwell.
 */
class ZoneHysteresis {
public:
    ZoneHysteresis(float deadband, uint8_t dwell) : mDeadband(deadband), mDwell(dwell) {}

    void reset();
    Zone update(float value, float low, float high);
    Zone zone() const { return mZone; }

private:
    Zone candidate(float value, float low, float high) const;

    float   mDeadband;
    uint8_t mDwell;
    Zone    mZone    = Zone::Unknown;
    Zone    mPending = Zone::Unknown;
    uint8_t mSeen    = 0;
};

class SpeedGauge {
public:
    /// 200 m. Below about 0.8 m/s, where 200 m takes longer than kHistory
    /// seconds, the window is the last kHistory - 1 seconds instead.
    static constexpr uint32_t kWindowCm       = 20000;
    static constexpr float    kBlendTauSec    = 30.0f;
    static constexpr float    kDeadbandSecPerKm = 3.0f;
    static constexpr uint8_t  kDwellTicks     = 2;
    static constexpr uint8_t  kHoldOffSec     = 5;
    static constexpr size_t   kHistory        = 256;  ///< Seconds of distance kept.

    /// Start a new step with target band [lowMps, highMps].
    void startStep(float lowMps, float highMps);

    /// Start a new step with no target: the value and the step average are
    /// kept up, and the zone stays Unknown (NoSignal without a reading).
    void startStep();

    /// One active second. @p liveMps and @p liveValid: the current live
    /// speed and whether there is a current reading. @p stepDistanceCm: the
    /// step's distance so far.
    void tick(float liveMps, bool liveValid, uint32_t stepDistanceCm);

    /// The run resumed after a pause: start the blend again from live speed
    /// and hold the zone off. The distance window is kept.
    void resume();

    float valueMps() const { return mValue; }
    Zone  zone() const { return mZone; }

    /// The step's average speed so far: distance / active time.
    float stepAverageMps() const { return mStepAverage; }

private:
    void restartStep();
    void restartBlend();

    float          mLow = 0.0f, mHigh = 0.0f;
    bool           mTargeted = false;
    uint32_t       mDist[kHistory] = {};  ///< Step distance at each second, ring.
    uint32_t       mSeconds = 0;          ///< Active seconds in the step.
    uint32_t       mBlendSec = 0;
    uint8_t        mHoldOff = 0;
    bool           mHadSignal = true;
    float          mValue = 0.0f;
    float          mStepAverage = 0.0f;
    Zone           mZone = Zone::Unknown;
    ZoneHysteresis mHyst{kDeadbandSecPerKm, kDwellTicks};
};

class HeartRateGauge {
public:
    static constexpr float   kDeadbandBpm = 3.0f;
    static constexpr uint8_t kDwellTicks  = 2;
    static constexpr uint8_t kHoldOffSec  = 5;

    /// Start a new step with target band [lowBpm, highBpm].
    void startStep(float lowBpm, float highBpm);

    /// Start a new step with no target: the value is kept up, and the zone
    /// stays Unknown (NoSignal without a reading).
    void startStep();

    /// One active second. @p valid false when there is no trustworthy reading.
    void tick(float bpm, bool valid);

    /// The run resumed after a pause: hold the zone off.
    void resume();

    float valueBpm() const { return mValue; }
    Zone  zone() const { return mZone; }

private:
    void restart();

    float          mLow = 0.0f, mHigh = 0.0f;
    bool           mTargeted = false;
    uint8_t        mHoldOff = 0;
    bool           mHadSignal = true;
    float          mValue = 0.0f;
    Zone           mZone = Zone::Unknown;
    ZoneHysteresis mHyst{kDeadbandBpm, kDwellTicks};
};

/**
 * Position on a gauge arc, 0..1, for @p value against band [low, high]: the
 * band fills the middle third whatever its width, each outer third covers
 * @p margin beyond its edge, and the value pins at 0 or 1 beyond that.
 * Low values are at 0, and a NaN value sits mid-arc. For pace, where lower is faster, pass pace and flip
 * the result if fast should be on the right.
 */
float arcFraction(float value, float low, float high, float margin);

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_TARGET_GAUGE_HPP
