/**
 ******************************************************************************
 * @file    ResettableMonotonicCounter.hpp
 * @date    11-May-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Resettable monotonic counter for tracking cumulative metrics
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SDK_METRICS_RESETTABLE_MONOTONIC_COUNTER_HPP
#define SDK_METRICS_RESETTABLE_MONOTONIC_COUNTER_HPP

#include <cstdint>
#include <type_traits>

namespace SDK::Metric {

/**
 * @brief Monotonic counter that allows the input value to reset.
 *
 * Tracks cumulative metrics whose sensor value normally increases over time,
 * but can legally reset to zero or another lower value.
 * Suitable for: step counters that reset at midnight, daily counters,
 *               counters reset by the sensor/driver.
 * NOT suitable for: speed, heart rate, cadence, pace, temperature,
 *                   altitude ascent/descent tracking.
 *
 * Provides both active values (excluding pauses) and total values
 * (including pauses).
 *
 * Difference from MonotonicCounter:
 * - MonotonicCounter ignores values smaller than the previous value.
 * - ResettableMonotonicCounter treats a smaller value as a counter reset and
 *   continues accumulation from the new sensor value.
 *
 * Example:
 *   sensor : 1000 -> 1010 -> 1020 ->  0 ->  5 -> 12
 *   delta  :           10      10     0     5     7
 *   total  :    0 ->   10 ->   20 -> 20 -> 25 -> 32
 *
 * @tparam T Arithmetic type for counter values (uint32_t, int, float, etc.)
 */

    template<typename T>
class ResettableMonotonicCounter {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");

public:
    /**
     * @brief Construct and zero-initialize all state.
     *        Call init() before use.
     */
    ResettableMonotonicCounter();

    ~ResettableMonotonicCounter() = default;

    /**
     * @brief Initialize the counter and reset all state.
     *        Must be called before add().
     */
    void init();

    /**
     * @brief Reset all accumulated values (total and lap, active and including-pauses).
     */
    void reset();

    /**
     * @brief Reset current lap values only. Total values are preserved.
     */
    void resetLap();

    /**
     * @brief Add new metric measurement.
     *
     * First call after init/reset sets the base point.
     * If currentValue is smaller than the previous value, it is treated as a
     * sensor counter reset and currentValue is used as the next delta.
     *
     * @param currentValue Current absolute metric value from sensor
     */
    void add(T currentValue);

    /**
     * @brief Get total active value (excluding pauses).
     */
    T getValueActive() const;

    /**
     * @brief Get total value including pauses.
     */
    T getValueTotal() const;

    /**
     * @brief Get current lap active value (excluding pauses).
     */
    T getLapValueActive() const;

    /**
     * @brief Get current lap total value including pauses.
     */
    T getLapValueTotal() const;

    /**
     * @brief Get the most recent value added via add().
     */
    T getCurrent() const;

    /**
     * @brief Pause metric tracking. Active value stops accumulating.
     */
    void pause();

    /**
     * @brief Resume metric tracking after pause.
     */
    void resume();

    /**
     * @brief Returns true if at least one measurement has been added after init/reset.
     */
    bool isValid() const;

    /**
     * @brief Returns true if at least one measurement has been added after last lap reset.
     */
    bool isLapValid() const;

    /**
     * @brief Returns true if counter is currently paused.
     */
    bool isPaused() const;

private:
    T getDelta(T currentValue) const;

private:
    T    mValueActive;     /* Accumulated active value from start (excluding pauses) */
    T    mValueTotal;      /* Accumulated total value from start (including pauses) */
    T    mLapValueActive;  /* Accumulated active value for current lap */
    T    mLapValueTotal;   /* Accumulated total value for current lap */
    T    mLastValidValue;  /* Last accepted sensor value */
    bool mIsInitialized;   /* True after init() has been called */
    bool mIsPaused;        /* True while tracking is paused */
    bool mHasData;         /* True after the first add() since init/reset */
    bool mHasLapData;      /* True after the first add() since last resetLap() */
};

// Template implementation

template<typename T>
ResettableMonotonicCounter<T>::ResettableMonotonicCounter()
    : mValueActive(T{})
    , mValueTotal(T{})
    , mLapValueActive(T{})
    , mLapValueTotal(T{})
    , mLastValidValue(T{})
    , mIsInitialized(false)
    , mIsPaused(false)
    , mHasData(false)
    , mHasLapData(false)
{
}

template<typename T>
void ResettableMonotonicCounter<T>::init()
{
    mValueActive    = T{};
    mValueTotal     = T{};
    mLapValueActive = T{};
    mLapValueTotal  = T{};
    mLastValidValue = T{};
    mIsInitialized  = true;
    mIsPaused       = false;
    mHasData        = false;
    mHasLapData     = false;
}

template<typename T>
void ResettableMonotonicCounter<T>::reset()
{
    mValueActive    = T{};
    mValueTotal     = T{};
    mLapValueActive = T{};
    mLapValueTotal  = T{};
    mLastValidValue = T{};
    mIsPaused       = false;
    mHasData        = false;
    mHasLapData     = false;
}

template<typename T>
void ResettableMonotonicCounter<T>::resetLap()
{
    mLapValueActive = T{};
    mLapValueTotal  = T{};
    mHasLapData     = false;
}

template<typename T>
T ResettableMonotonicCounter<T>::getDelta(T currentValue) const
{
    if (currentValue < mLastValidValue) {
        // Sensor counter was reset. Example: 2350 -> 0 -> 7.
        // The value after reset already represents the new delta since reset.
        return currentValue;
    }

    return currentValue - mLastValidValue;
}

template<typename T>
void ResettableMonotonicCounter<T>::add(T currentValue)
{
    if (!mIsInitialized) {
        return;
    }

    if (!mHasData) {
        mLastValidValue = currentValue;
        mHasData        = true;
        mHasLapData     = true;
        return;
    }

    const T delta = getDelta(currentValue);

    mValueTotal += delta;

    if (!mHasLapData) {
        mHasLapData = true;
    }
    mLapValueTotal += delta;

    if (!mIsPaused) {
        mValueActive    += delta;
        mLapValueActive += delta;
    }

    mLastValidValue = currentValue;
}

template<typename T>
T ResettableMonotonicCounter<T>::getValueActive() const { return mValueActive; }

template<typename T>
T ResettableMonotonicCounter<T>::getValueTotal() const { return mValueTotal; }

template<typename T>
T ResettableMonotonicCounter<T>::getLapValueActive() const { return mLapValueActive; }

template<typename T>
T ResettableMonotonicCounter<T>::getLapValueTotal() const { return mLapValueTotal; }

template<typename T>
T ResettableMonotonicCounter<T>::getCurrent() const { return mLastValidValue; }

template<typename T>
void ResettableMonotonicCounter<T>::pause()
{
    if (!mIsInitialized || mIsPaused) {
        return;
    }
    mIsPaused = true;
}

template<typename T>
void ResettableMonotonicCounter<T>::resume()
{
    if (!mIsInitialized || !mIsPaused) {
        return;
    }
    mIsPaused = false;
}

template<typename T>
bool ResettableMonotonicCounter<T>::isValid() const { return mHasData; }

template<typename T>
bool ResettableMonotonicCounter<T>::isLapValid() const { return mHasLapData; }

template<typename T>
bool ResettableMonotonicCounter<T>::isPaused() const { return mIsPaused; }

}  // namespace SDK::Metric

#endif // SDK_METRICS_RESETTABLE_MONOTONIC_COUNTER_HPP
