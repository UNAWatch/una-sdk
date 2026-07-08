/**
 * @file MonotonicCounter.hpp
 * @date 06-02-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Monotonic counter template for tracking cumulative metrics
 */

#ifndef SDK_METRICS_MONOTONIC_COUNTER_HPP
#define SDK_METRICS_MONOTONIC_COUNTER_HPP

#include <cstdint>
#include <type_traits>

namespace SDK::Metric {

/**
 * @brief Universal monotonic counter template for activity tracking
 *
 * Tracks cumulative metrics that monotonically increase over time.
 * Suitable for: distance, steps, floors, calories, time duration.
 * NOT suitable for: speed, heart rate, cadence, pace.
 *                   Altitude tracking with ascent/descent accumulation.
 *
 * Provides both active values (excluding pauses) and total values (including pauses).
 * Handles pause/resume to avoid sensor jumps affecting calculations.
 *
 * @tparam T Arithmetic type for counter values (int, uint32_t, float, double, etc.)
 */
template<typename T>
class MonotonicCounter {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");

public:
    /**
     * @brief Construct and zero-initialize all state.
     *        Call init() before use.
     */
    MonotonicCounter();

    ~MonotonicCounter() = default;

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
     * @brief Close the current lap as exactly @p span and carry the overshoot
     *        into the next lap.
     *
     * Unlike resetLap(), which re-bases at the next sample and therefore
     * discards whatever was accumulated beyond the lap target, advanceLap()
     * shifts the lap base forward by @p span. The amount already accumulated
     * past @p span becomes the starting value of the new lap, so distance-
     * triggered laps stay pinned to exact multiples of the target instead of
     * drifting by the per-sample overshoot.
     *
     * @param span Lap size to close on, in the counter's value units.
     */
    void advanceLap(T span);

    /**
     * @brief Add new metric measurement.
     *
     * First call after init/reset sets the base point.
     * Subsequent calls with values smaller than the previous are ignored (sensor errors).
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
     *        Adjusts base points to exclude the metric accumulated during the pause.
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
    T    mValueActive;        /* Accumulated active value from start (excluding pauses) */
    T    mValueTotal;         /* Accumulated total value from start (including pauses) */
    T    mLapValueActive;     /* Accumulated active value for current lap */
    T    mLapValueTotal;      /* Accumulated total value for current lap */
    T    mBaseValue;          /* Sensor base for active accumulation */
    T    mBaseValueTotal;     /* Sensor base for total accumulation */
    T    mLapBaseValue;       /* Sensor base for lap active accumulation */
    T    mLapBaseValueTotal;  /* Sensor base for lap total accumulation */
    T    mLastValidValue;     /* Last accepted sensor value */
    T    mPauseStartValue;    /* Sensor value at the moment pause() was called */
    bool mIsInitialized;      /* True after init() has been called */
    bool mIsPaused;           /* True while tracking is paused */
    bool mHasData;            /* True after the first add() since init/reset */
    bool mHasLapData;         /* True after the first add() since last resetLap() */
};

// Template implementation

template<typename T>
MonotonicCounter<T>::MonotonicCounter()
    : mValueActive(T{})
    , mValueTotal(T{})
    , mLapValueActive(T{})
    , mLapValueTotal(T{})
    , mBaseValue(T{})
    , mBaseValueTotal(T{})
    , mLapBaseValue(T{})
    , mLapBaseValueTotal(T{})
    , mLastValidValue(T{})
    , mPauseStartValue(T{})
    , mIsInitialized(false)
    , mIsPaused(false)
    , mHasData(false)
    , mHasLapData(false)
{
}

template<typename T>
void MonotonicCounter<T>::init()
{
    mValueActive       = T{};
    mValueTotal        = T{};
    mLapValueActive    = T{};
    mLapValueTotal     = T{};
    mBaseValue         = T{};
    mBaseValueTotal    = T{};
    mLapBaseValue      = T{};
    mLapBaseValueTotal = T{};
    mLastValidValue    = T{};
    mPauseStartValue   = T{};
    mIsInitialized     = true;
    mIsPaused          = false;
    mHasData           = false;
    mHasLapData        = false;
}

template<typename T>
void MonotonicCounter<T>::reset()
{
    mValueActive       = T{};
    mValueTotal        = T{};
    mLapValueActive    = T{};
    mLapValueTotal     = T{};
    mBaseValue         = T{};
    mBaseValueTotal    = T{};
    mLapBaseValue      = T{};
    mLapBaseValueTotal = T{};
    mLastValidValue    = T{};
    mPauseStartValue   = T{};
    mIsPaused          = false;
    mHasData           = false;
    mHasLapData        = false;
}

template<typename T>
void MonotonicCounter<T>::resetLap()
{
    mLapValueActive    = T{};
    mLapValueTotal     = T{};
    mLapBaseValue      = T{};
    mLapBaseValueTotal = T{};
    mHasLapData        = false;
}

template<typename T>
void MonotonicCounter<T>::advanceLap(T span)
{
    // Move the lap base forward by exactly `span`; the excess already covered
    // this lap (the overshoot past the target) rolls into the new lap so lap
    // boundaries never drift. Base stays seated, so add() keeps accumulating
    // instead of re-seating on the next sample.
    mLapValueActive    = mLapValueActive    - span;
    mLapValueTotal     = mLapValueTotal     - span;
    mLapBaseValue      = mLapBaseValue      + span;
    mLapBaseValueTotal = mLapBaseValueTotal + span;
}

template<typename T>
void MonotonicCounter<T>::add(T currentValue)
{
    if (!mIsInitialized) {
        return;
    }

    if (mBaseValue == T{} && mValueActive == T{}) {
        mBaseValue         = currentValue;
        mBaseValueTotal    = currentValue;
        mLapBaseValue      = currentValue;
        mLapBaseValueTotal = currentValue;
        mLastValidValue    = currentValue;
        mHasData           = true;
        mHasLapData        = true;
        return;
    }

    if (mLapBaseValue == T{} && mLapValueActive == T{}) {
        mLapBaseValue      = currentValue;
        mLapBaseValueTotal = currentValue;
        mHasLapData        = true;
    }

    if (currentValue < mLastValidValue) {
        return;
    }

    mValueTotal    = currentValue - mBaseValueTotal;
    mLapValueTotal = currentValue - mLapBaseValueTotal;

    if (mIsPaused) {
        mLastValidValue = currentValue;
        return;
    }

    mValueActive    = currentValue - mBaseValue;
    mLapValueActive = currentValue - mLapBaseValue;
    mLastValidValue = currentValue;
}

template<typename T>
T MonotonicCounter<T>::getValueActive() const { return mValueActive; }

template<typename T>
T MonotonicCounter<T>::getValueTotal() const { return mValueTotal; }

template<typename T>
T MonotonicCounter<T>::getLapValueActive() const { return mLapValueActive; }

template<typename T>
T MonotonicCounter<T>::getLapValueTotal() const { return mLapValueTotal; }

template<typename T>
T MonotonicCounter<T>::getCurrent() const { return mLastValidValue; }

template<typename T>
void MonotonicCounter<T>::pause()
{
    if (!mIsInitialized || mIsPaused) {
        return;
    }
    mIsPaused        = true;
    mPauseStartValue = mLastValidValue;
}

template<typename T>
void MonotonicCounter<T>::resume()
{
    if (!mIsInitialized || !mIsPaused) {
        return;
    }
    T pauseOffset  = mLastValidValue - mPauseStartValue;
    mBaseValue     = mBaseValue    + pauseOffset;
    mLapBaseValue  = mLapBaseValue + pauseOffset;
    mIsPaused      = false;
}

template<typename T>
bool MonotonicCounter<T>::isValid() const { return mHasData; }

template<typename T>
bool MonotonicCounter<T>::isLapValid() const { return mHasLapData; }

template<typename T>
bool MonotonicCounter<T>::isPaused() const { return mIsPaused; }

}  // namespace SDK::Metric

#endif // SDK_METRICS_MONOTONIC_COUNTER_HPP
