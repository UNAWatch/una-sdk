/**
 * @file DeltaCounter.hpp
 * @date 07-02-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Delta counter for tracking metrics with positive and negative changes
 */

#ifndef SDK_METRICS_DELTA_COUNTER_HPP
#define SDK_METRICS_DELTA_COUNTER_HPP

#include <cstdint>
#include <limits>
#include <algorithm>

namespace SDK::Metric {

/**
 * @brief Delta counter for tracking cumulative changes
 *
 * Tracks metrics that can both increase and decrease over time.
 * Suitable for: altitude, elevation.
 * NOT suitable for: cumulative one-direction metrics (distance, steps).
 *                   Statistics such as average/min/max.
 *
 * Provides:
 * - Current value and net delta from start
 * - Ascent (sum of positive changes) and descent (sum of negative changes)
 * - Separate tracking for total activity and current lap
 * - Pause/resume support - affects ascent/descent accumulation only, delta is always updated
 *
 * @note This class does NOT perform filtering. Input values should be
 *       pre-filtered before calling add().
 */
class DeltaCounter {
public:
    /**
     * @brief Construct and zero-initialize all state.
     *        Call init() before use.
     */
    DeltaCounter();

    ~DeltaCounter() = default;

    /**
     * @brief Initialize the delta counter.
     *
     * @param minChange Minimum change threshold to register as ascent or descent (e.g., 1.0m).
     *                  Changes smaller than this are accumulated but not yet registered,
     *                  reducing noise. Must be non-negative.
     * @return true on success, false if minChange < 0
     */
    bool init(float minChange = 1.0f);

    /**
     * @brief Reset all calculations (current, delta, ascent, descent for total and lap).
     */
    void reset();

    /**
     * @brief Reset current lap values only (delta, ascent, descent). Total values are preserved.
     */
    void resetLap();

    /**
     * @brief Add new measurement.
     *
     * First call after init/reset sets the start value.
     * During pause, current value is updated but ascent/descent are not accumulated.
     *
     * @param currentValue Current metric value (e.g., altitude in metres)
     */
    void add(float currentValue);

    /**
     * @brief Get the most recent value added via add().
     */
    float getCurrent() const;

    /**
     * @brief Get net delta from start (current minus start value).
     *        Can be positive or negative. Returns 0.0 if no data.
     *        Not affected by pause.
     */
    float getDelta() const;

    /**
     * @brief Get net lap delta (current minus lap start value).
     *        Returns 0.0 if no lap data.
     */
    float getLapDelta() const;

    /**
     * @brief Get total ascent (sum of positive changes >= minChange during active periods).
     */
    float getAscent() const;

    /**
     * @brief Get total descent (sum of negative changes >= minChange during active periods).
     *        Always returned as a positive value.
     */
    float getDescent() const;

    /**
     * @brief Get current lap ascent.
     */
    float getLapAscent() const;

    /**
     * @brief Get current lap descent. Always returned as a positive value.
     */
    float getLapDescent() const;

    /**
     * @brief Pause tracking. Stops ascent/descent accumulation; delta continues to update.
     */
    void pause();

    /**
     * @brief Resume tracking after pause. Rebases the ascent/descent thresholds onto
     *        the current value, so the change during the pause is not accumulated.
     */
    void resume();

    /**
     * @brief Returns true if at least one measurement has been added after init/reset.
     */
    bool isValid() const;

    /**
     * @brief Returns true if at least one measurement has been added after last resetLap().
     */
    bool isLapValid() const;

    /**
     * @brief Returns true if counter is currently paused.
     */
    bool isPaused() const;

private:
    float mMinChange;     /* Minimum change threshold for registering ascent/descent */
    float mCurrent;       /* Most recent value passed to add() */
    float mStartValue;    /* Value at first add(); base for getDelta() */
    float mLapStartValue; /* Value at last resetLap(); base for getLapDelta() */
    float mAscent;        /* Cumulative ascent during active periods */
    float mDescent;       /* Cumulative descent during active periods (stored as positive) */
    float mLapAscent;     /* Cumulative lap ascent during active periods */
    float mLapDescent;    /* Cumulative lap descent during active periods (stored as positive) */
    float mLastValue;     /* Last value used as base for total ascent/descent threshold */
    float mLapLastValue;  /* Last value used as base for lap ascent/descent threshold */
    bool  mIsInitialized; /* True after init() has been called */
    bool  mIsPaused;      /* True while tracking is paused */
    bool  mHasData;       /* True after the first add() since init/reset */
    bool  mHasLapData;    /* True after the first add() since last resetLap() */
};

// Implementation

inline DeltaCounter::DeltaCounter()
    : mMinChange(1.0f)
    , mCurrent(0.0f)
    , mStartValue(0.0f)
    , mLapStartValue(0.0f)
    , mAscent(0.0f)
    , mDescent(0.0f)
    , mLapAscent(0.0f)
    , mLapDescent(0.0f)
    , mLastValue(0.0f)
    , mLapLastValue(0.0f)
    , mIsInitialized(false)
    , mIsPaused(false)
    , mHasData(false)
    , mHasLapData(false)
{
}

inline bool DeltaCounter::init(float minChange)
{
    if (minChange < 0.0f) {
        return false;
    }
    mMinChange     = minChange;
    mCurrent       = 0.0f;
    mStartValue    = 0.0f;
    mLapStartValue = 0.0f;
    mAscent        = 0.0f;
    mDescent       = 0.0f;
    mLapAscent     = 0.0f;
    mLapDescent    = 0.0f;
    mLastValue     = 0.0f;
    mLapLastValue  = 0.0f;
    mIsInitialized = true;
    mIsPaused      = false;
    mHasData       = false;
    mHasLapData    = false;
    return true;
}

inline void DeltaCounter::reset()
{
    mCurrent       = 0.0f;
    mStartValue    = 0.0f;
    mLapStartValue = 0.0f;
    mAscent        = 0.0f;
    mDescent       = 0.0f;
    mLapAscent     = 0.0f;
    mLapDescent    = 0.0f;
    mLastValue     = 0.0f;
    mLapLastValue  = 0.0f;
    mIsPaused      = false;
    mHasData       = false;
    mHasLapData    = false;
}

inline void DeltaCounter::resetLap()
{
    mLapStartValue = mCurrent;
    mLapAscent     = 0.0f;
    mLapDescent    = 0.0f;
    mLapLastValue  = mCurrent;
    mHasLapData    = false;
}

inline void DeltaCounter::add(float currentValue)
{
    if (!mIsInitialized) {
        return;
    }

    mCurrent = currentValue;

    if (!mHasData) {
        mStartValue    = currentValue;
        mLapStartValue = currentValue;
        mLastValue     = currentValue;
        mLapLastValue  = currentValue;
        mHasData       = true;
        mHasLapData    = true;
        return;
    }

    /* Re-arm after resetLap(); mLapStartValue stays at the lap boundary */
    mHasLapData = true;

    if (mIsPaused) {
        return;
    }

    float change    = currentValue - mLastValue;
    float lapChange = currentValue - mLapLastValue;

    if (change >= mMinChange) {
        mAscent   += change;
        mLastValue = currentValue;
    } else if (change <= -mMinChange) {
        mDescent  += (-change);
        mLastValue = currentValue;
    }

    if (lapChange >= mMinChange) {
        mLapAscent   += lapChange;
        mLapLastValue = currentValue;
    } else if (lapChange <= -mMinChange) {
        mLapDescent  += (-lapChange);
        mLapLastValue = currentValue;
    }
}

inline float DeltaCounter::getCurrent() const { return mCurrent; }

inline float DeltaCounter::getDelta() const
{
    return mHasData ? mCurrent - mStartValue : 0.0f;
}

inline float DeltaCounter::getLapDelta() const
{
    return mHasLapData ? mCurrent - mLapStartValue : 0.0f;
}

inline float DeltaCounter::getAscent()     const { return mAscent; }
inline float DeltaCounter::getDescent()    const { return mDescent; }
inline float DeltaCounter::getLapAscent()  const { return mLapAscent; }
inline float DeltaCounter::getLapDescent() const { return mLapDescent; }

inline void DeltaCounter::pause()
{
    if (!mIsInitialized || mIsPaused) {
        return;
    }
    mIsPaused = true;
}

inline void DeltaCounter::resume()
{
    if (!mIsInitialized || !mIsPaused) {
        return;
    }

    /* Rebase the thresholds; the change during the pause must not accumulate */
    if (mHasData) {
        mLastValue    = mCurrent;
        mLapLastValue = mCurrent;
    }

    mIsPaused = false;
}

inline bool DeltaCounter::isValid()    const { return mHasData; }
inline bool DeltaCounter::isLapValid() const { return mHasLapData; }
inline bool DeltaCounter::isPaused()   const { return mIsPaused; }

}  // namespace SDK::Metric

#endif // SDK_METRICS_DELTA_COUNTER_HPP
