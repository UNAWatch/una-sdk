/**
 * @file ThrottledSample.hpp
 * @date 26-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Throttled sample - caches the latest value and controls emit rate.
 */

#ifndef SDK_METRICS_THROTTLED_SAMPLE_HPP
#define SDK_METRICS_THROTTLED_SAMPLE_HPP

#include <cstdint>

namespace SDK::Metric {

/**
 * @brief Caches the latest sampled value and throttles how often it is emitted.
 *
 * Useful for sensor values that arrive frequently but only need to be
 * processed (logged, transmitted, displayed) periodically.
 *
 * The period is specified in milliseconds; time is sourced from a TClock
 * object via getTimeMs(). Handles 32-bit millisecond wrap-around correctly.
 *
 * @tparam T       Value type (float, uint8_t, uint32_t, ...).
 * @tparam TClock  Clock source type. Must provide: @code uint32_t getTimeMs() @endcode
 */
template<typename T, typename TClock>
class ThrottledSample
{
public:
    /**
     * @brief Construct with a clock source.
     * @param clock  Reference to a clock that provides getTimeMs().
     *               Must remain valid for the lifetime of this object.
     */
    explicit ThrottledSample(TClock& clock)
        : mClock(clock)
    {}

    /**
     * @brief Reset state and start the periodic countdown.
     * @param periodMs  Interval between automatic emissions, in milliseconds.
     *
     * Clears the stored value, marks it invalid, and pre-sets the pending flag
     * so the first valid value is emitted without waiting a full period.
     */
    void reset(uint32_t periodMs)
    {
        mPeriodMs = periodMs;
        mLastMs   = mClock.getTimeMs();
        mValue    = T{};
        mIsValid  = false;
        mPending  = true;   // emit first valid value immediately
    }

    /**
     * @brief Store a new value and mark it valid.
     * @param value  New sample value.
     */
    void set(T value)
    {
        mValue   = value;
        mIsValid = true;
    }

    /**
     * @brief Return the stored value.
     * @return  Last value passed to set(), or T{} if not yet set.
     */
    T get() const { return mValue; }

    /**
     * @brief True if at least one value has been stored since the last reset().
     */
    bool isValid() const { return mIsValid; }

    /**
     * @brief Request an out-of-schedule emit.
     *
     * The next isDue() call will return true (provided a value is valid).
     */
    void request() { mPending = true; }

    /**
     * @brief Check whether the value is due to be processed.
     *
     * Advances the internal timer. Returns true when:
     * - a valid value is available, AND
     * - the periodic interval has elapsed OR request() was called.
     *
     * @note Non-destructive: the pending flag is NOT cleared here.
     *       Call consume() after the value has been successfully processed.
     */
    bool isDue()
    {
        if (!mIsValid) {
            return false;
        }

        const uint32_t now = mClock.getTimeMs();
        if ((now - mLastMs) >= mPeriodMs) {
            mPending = true;
            mLastMs  = now;
        }

        return mPending;
    }

    /**
     * @brief Clear the pending flag.
     *
     * Call after the value has been successfully processed.
     */
    void consume() { mPending = false; }

private:
    TClock&  mClock;
    T        mValue{};
    bool     mIsValid  = false;
    bool     mPending  = false;
    uint32_t mPeriodMs = 0;
    uint32_t mLastMs   = 0;
};

} // namespace SDK::Metric

#endif // SDK_METRICS_THROTTLED_SAMPLE_HPP
