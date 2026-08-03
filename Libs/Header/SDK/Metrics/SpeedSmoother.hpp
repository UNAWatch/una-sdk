/**
 * @file SpeedSmoother.hpp
 * @brief Rolling-window smoother for the live speed / pace display
 */

#ifndef SDK_METRICS_SPEED_SMOOTHER_HPP
#define SDK_METRICS_SPEED_SMOOTHER_HPP

#include <cstddef>

namespace SDK::Metric {

/**
 * @brief Rolling-window mean of instantaneous speed, and the pace derived from it.
 *
 * A raw GPS speed sample carries enough noise (typically a few tenths of a m/s)
 * that the pace derived from a single sample swings by tens of seconds per
 * kilometre from one second to the next, which makes the live pace readout
 * useless for holding a target pace. This class averages the last
 * @p WindowTicks samples so the readout is stable enough to run to, while still
 * reflecting a genuine change of effort within the window length.
 *
 * Averaging the speed and then inverting (rather than averaging the pace) is
 * the physically meaningful operation: with uniform sampling the window mean
 * speed equals window distance / window time, so the smoothed pace is the pace
 * actually run over the window. Averaging pace samples would bias the result
 * toward the slow samples.
 *
 * @tparam WindowTicks Window length in ticks. tick() is expected to be called
 *                     at a fixed cadence (1 Hz in the activity apps), so this
 *                     is also the window length in seconds.
 *
 * @note This smooths the *displayed* value only. Recorded FIT speed, session
 *       averages and maxima should keep using the raw samples.
 */
template <std::size_t WindowTicks>
class SpeedSmoother {
    static_assert(WindowTicks > 0, "SpeedSmoother needs a non-empty window");

public:
    /**
     * @brief Construct and zero-initialize all state. Call init() before use.
     */
    SpeedSmoother()
        : mMinValid(0.0f)
        , mMaxValid(0.0f)
        , mSamples{}
        , mHasSample{}
        , mNext(0)
        , mIsInitialized(false)
    {
    }

    ~SpeedSmoother() = default;

    /**
     * @brief Initialize the smoother with a valid measurement range.
     *
     * @param minValid Speed (m/s) below which no pace is reported. Samples
     *                 below it still enter the window -- slowing to a walk must
     *                 pull the mean down -- but a window mean at or below this
     *                 floor yields no pace, matching the "---" readout.
     * @param maxValid Speed (m/s) above which a sample is discarded as bogus.
     *                 One spike would otherwise corrupt the whole window.
     * @return true on success, false if minValid >= maxValid
     */
    bool init(float minValid, float maxValid)
    {
        if (minValid >= maxValid) {
            return false;
        }
        mMinValid      = minValid;
        mMaxValid      = maxValid;
        mIsInitialized = true;
        reset();
        return true;
    }

    /**
     * @brief Discard the window. The valid range set by init() is preserved.
     *
     * Call this whenever the samples already in the window no longer describe
     * the current effort: at track start, and on resume after a pause.
     */
    void reset()
    {
        for (std::size_t i = 0; i < WindowTicks; i++) {
            mSamples[i]   = 0.0f;
            mHasSample[i] = false;
        }
        mNext = 0;
    }

    /**
     * @brief Advance the window by one tick.
     *
     * Must be called once per tick even when no speed is available, so that the
     * window ages: a lost fix empties it after WindowTicks ticks and the pace
     * goes unavailable instead of being held forward forever.
     *
     * @param speedMs Current speed in m/s (ignored when @p valid is false)
     * @param valid   true when @p speedMs comes from a current, trustworthy fix
     */
    void tick(float speedMs, bool valid)
    {
        if (!mIsInitialized) {
            return;
        }

        const bool usable = valid && speedMs >= 0.0f && speedMs <= mMaxValid;

        mSamples[mNext]   = usable ? speedMs : 0.0f;
        mHasSample[mNext] = usable;
        mNext             = (mNext + 1) % WindowTicks;
    }

    /**
     * @brief Window mean speed in m/s, or 0.0 when the window holds no samples.
     */
    float getSpeed() const
    {
        float       sum   = 0.0f;
        std::size_t count = 0;

        // Summed on demand rather than kept incrementally: at one tick per
        // second the cost is irrelevant, and it cannot accumulate drift.
        for (std::size_t i = 0; i < WindowTicks; i++) {
            if (mHasSample[i]) {
                sum += mSamples[i];
                count++;
            }
        }

        return (count > 0) ? (sum / static_cast<float>(count)) : 0.0f;
    }

    /**
     * @brief Smoothed pace in s/m, or 0.0 when no pace can be reported
     *        (empty window, or a mean at or below the minValid floor).
     */
    float getPace() const
    {
        const float speed = getSpeed();
        return (speed > mMinValid) ? (1.0f / speed) : 0.0f;
    }

    /**
     * @brief Number of samples currently in the window (0 .. WindowTicks).
     */
    std::size_t getSampleCount() const
    {
        std::size_t count = 0;
        for (std::size_t i = 0; i < WindowTicks; i++) {
            if (mHasSample[i]) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Returns true while the window holds at least one sample.
     */
    bool isValid() const { return getSampleCount() > 0; }

    /**
     * @brief Window length in ticks, as configured by the template argument.
     */
    static constexpr std::size_t getWindowTicks() { return WindowTicks; }

    /** @brief Minimum valid speed passed to init(). */
    float getMinValid() const { return mMinValid; }

    /** @brief Maximum valid speed passed to init(). */
    float getMaxValid() const { return mMaxValid; }

private:
    float       mMinValid;                /* Speed floor below which no pace is reported */
    float       mMaxValid;                /* Speed ceiling above which a sample is bogus */
    float       mSamples[WindowTicks];    /* Ring of the last WindowTicks samples */
    bool        mHasSample[WindowTicks];  /* Which ring slots hold a usable sample */
    std::size_t mNext;                    /* Ring slot the next tick() writes */
    bool        mIsInitialized;           /* True after init() has been called */
};

}  // namespace SDK::Metric

#endif // SDK_METRICS_SPEED_SMOOTHER_HPP
