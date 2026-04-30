/**
 * @file VariableCounter.hpp
 * @date 06-02-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Variable counter for tracking non-monotonic metrics
 */

#ifndef SDK_METRICS_VARIABLE_COUNTER_HPP
#define SDK_METRICS_VARIABLE_COUNTER_HPP

#include <cstdint>
#include <limits>

namespace SDK::Metric {

/**
 * @brief Universal variable counter for activity tracking
 *
 * Tracks metrics that can increase or decrease over time.
 * Suitable for: speed, heart rate, cadence, power, pace, altitude, temperature.
 * NOT suitable for: cumulative metrics that only increase (distance, steps, calories).
 *
 * Provides average, minimum, and maximum values for active periods (excluding pauses).
 * Uses a valid range [minValid, maxValid] to filter out-of-range sensor readings.
 *
 * @note Assumes uniform sampling; add() should be called at regular intervals
 *       for accurate average calculations.
 */
class VariableCounter {
public:
    /**
     * @brief Construct and zero-initialize all state.
     *        Call init() before use.
     */
    VariableCounter();

    ~VariableCounter() = default;

    /**
     * @brief Initialize the counter with a valid measurement range.
     *
     * @param minValid Minimum valid value threshold. Values below are excluded from statistics.
     * @param maxValid Maximum valid value threshold. Values above are excluded from statistics.
     * @return true on success, false if minValid >= maxValid
     */
    bool init(float minValid, float maxValid);

    /**
     * @brief Reset all statistics (averages, min, max) to initial state.
     *        Valid range set by init() is preserved.
     */
    void reset();

    /**
     * @brief Reset current lap statistics only. Total statistics are preserved.
     */
    void resetLap();

    /**
     * @brief Add new metric measurement.
     *
     * Values outside [minValid, maxValid] are excluded from all statistics.
     * Should be called at regular intervals for accurate average calculation.
     *
     * @param currentValue Current metric value from sensor
     */
    void add(float currentValue);

    /**
     * @brief Get average value during active periods (excluding pauses and out-of-range values).
     *        Returns 0.0 if no valid samples.
     */
    float getAverage() const;

    /**
     * @brief Get current lap average value during active periods.
     *        Returns 0.0 if no valid lap samples.
     */
    float getLapAverage() const;

    /**
     * @brief Get minimum value observed during active periods.
     *        Returns 0.0 if no valid data.
     */
    float getMinimum() const;

    /**
     * @brief Get maximum value observed during active periods.
     *        Returns 0.0 if no valid data.
     */
    float getMaximum() const;

    /**
     * @brief Get minimum value in current lap during active periods.
     *        Returns 0.0 if no valid lap data.
     */
    float getLapMinimum() const;

    /**
     * @brief Get maximum value in current lap during active periods.
     *        Returns 0.0 if no valid lap data.
     */
    float getLapMaximum() const;

    /**
     * @brief Get the most recent value added via add().
     *        Includes out-of-range values; returns 0.0 if no data.
     */
    float getCurrent() const;

    /**
     * @brief Get minimum valid threshold set in init().
     */
    float getMinValid() const;

    /**
     * @brief Get maximum valid threshold set in init().
     */
    float getMaxValid() const;

    /**
     * @brief Pause metric tracking. Statistics accumulation stops until resume().
     */
    void pause();

    /**
     * @brief Resume metric tracking after pause.
     */
    void resume();

    /**
     * @brief Returns true if at least one valid measurement has been added after init/reset.
     */
    bool isValid() const;

    /**
     * @brief Returns true if at least one valid measurement has been added after last lap reset.
     */
    bool isLapValid() const;

    /**
     * @brief Returns true if counter is currently paused.
     */
    bool isPaused() const;

private:
    float    mMinValid;       /* Minimum valid value threshold (set by init) */
    float    mMaxValid;       /* Maximum valid value threshold (set by init) */
    float    mSumActive;      /* Sum of valid values during active periods */
    uint32_t mCountActive;    /* Count of valid samples during active periods */
    float    mLapSumActive;   /* Sum of valid values during active periods for current lap */
    uint32_t mLapCountActive; /* Count of valid lap samples during active periods */
    float    mMinValue;       /* Minimum valid value observed during active periods */
    float    mMaxValue;       /* Maximum valid value observed during active periods */
    float    mLapMinValue;    /* Minimum valid value in current lap */
    float    mLapMaxValue;    /* Maximum valid value in current lap */
    float    mCurrentValue;   /* Most recent value passed to add() */
    bool     mIsInitialized;  /* True after init() has been called */
    bool     mIsPaused;       /* True while tracking is paused */
    bool     mHasData;        /* True after the first valid add() since init/reset */
    bool     mHasLapData;     /* True after the first valid add() since last resetLap() */
};

// Implementation

inline VariableCounter::VariableCounter()
    : mMinValid(0.0f)
    , mMaxValid(0.0f)
    , mSumActive(0.0f)
    , mCountActive(0)
    , mLapSumActive(0.0f)
    , mLapCountActive(0)
    , mMinValue(std::numeric_limits<float>::max())
    , mMaxValue(0.0f)
    , mLapMinValue(std::numeric_limits<float>::max())
    , mLapMaxValue(0.0f)
    , mCurrentValue(0.0f)
    , mIsInitialized(false)
    , mIsPaused(false)
    , mHasData(false)
    , mHasLapData(false)
{
}

inline bool VariableCounter::init(float minValid, float maxValid)
{
    if (minValid >= maxValid) {
        return false;
    }
    mMinValid       = minValid;
    mMaxValid       = maxValid;
    mSumActive      = 0.0f;
    mCountActive    = 0;
    mLapSumActive   = 0.0f;
    mLapCountActive = 0;
    mMinValue       = std::numeric_limits<float>::max();
    mMaxValue       = 0.0f;
    mLapMinValue    = std::numeric_limits<float>::max();
    mLapMaxValue    = 0.0f;
    mCurrentValue   = 0.0f;
    mIsInitialized  = true;
    mIsPaused       = false;
    mHasData        = false;
    mHasLapData     = false;
    return true;
}

inline void VariableCounter::reset()
{
    mSumActive      = 0.0f;
    mCountActive    = 0;
    mLapSumActive   = 0.0f;
    mLapCountActive = 0;
    mMinValue       = std::numeric_limits<float>::max();
    mMaxValue       = 0.0f;
    mLapMinValue    = std::numeric_limits<float>::max();
    mLapMaxValue    = 0.0f;
    mCurrentValue   = 0.0f;
    mIsPaused       = false;
    mHasData        = false;
    mHasLapData     = false;
}

inline void VariableCounter::resetLap()
{
    mLapSumActive   = 0.0f;
    mLapCountActive = 0;
    mLapMinValue    = std::numeric_limits<float>::max();
    mLapMaxValue    = 0.0f;
    mHasLapData     = false;
}

inline void VariableCounter::add(float currentValue)
{
    if (!mIsInitialized) {
        return;
    }

    mCurrentValue = currentValue;

    if (currentValue < mMinValid || currentValue > mMaxValid) {
        return;
    }

    if (mIsPaused) {
        return;
    }

    mHasData    = true;
    mHasLapData = true;

    mMinValue    = std::min(mMinValue,    currentValue);
    mMaxValue    = std::max(mMaxValue,    currentValue);
    mLapMinValue = std::min(mLapMinValue, currentValue);
    mLapMaxValue = std::max(mLapMaxValue, currentValue);

    mSumActive      += currentValue;
    mCountActive++;
    mLapSumActive   += currentValue;
    mLapCountActive++;
}

inline float VariableCounter::getAverage() const
{
    return mCountActive > 0 ? mSumActive / static_cast<float>(mCountActive) : 0.0f;
}

inline float VariableCounter::getLapAverage() const
{
    return mLapCountActive > 0 ? mLapSumActive / static_cast<float>(mLapCountActive) : 0.0f;
}

inline float VariableCounter::getMinimum() const
{
    return (mHasData && mMinValue != std::numeric_limits<float>::max()) ? mMinValue : 0.0f;
}

inline float VariableCounter::getMaximum() const
{
    return mHasData ? mMaxValue : 0.0f;
}

inline float VariableCounter::getLapMinimum() const
{
    return (mLapMinValue != std::numeric_limits<float>::max()) ? mLapMinValue : 0.0f;
}

inline float VariableCounter::getLapMaximum() const
{
    return mLapMaxValue;
}

inline float VariableCounter::getCurrent()  const { return mCurrentValue; }
inline float VariableCounter::getMinValid() const { return mMinValid; }
inline float VariableCounter::getMaxValid() const { return mMaxValid; }

inline void VariableCounter::pause()
{
    if (!mIsInitialized || mIsPaused) {
        return;
    }
    mIsPaused = true;
}

inline void VariableCounter::resume()
{
    if (!mIsInitialized || !mIsPaused) {
        return;
    }
    mIsPaused = false;
}

inline bool VariableCounter::isValid()    const { return mHasData; }
inline bool VariableCounter::isLapValid() const { return mHasLapData; }
inline bool VariableCounter::isPaused()   const { return mIsPaused; }

}  // namespace SDK::Metric

#endif // SDK_METRICS_VARIABLE_COUNTER_HPP
