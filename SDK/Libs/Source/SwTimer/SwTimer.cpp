/**
 ******************************************************************************
 * @file    SwTimer.cpp
 * @date    17-September-2025
 * @author  Oleksandr Tymoshenko
 * @brief   Software timer implementation
 *
 * Provides a lightweight software timer utility based on the system
 * millisecond tick from IKernel. Supports start/stop, interval checking,
 * expiration query, and simple busy-wait delays.
 *
 ******************************************************************************
 */

#include "SDK/SwTimer/SwTimer.hpp"


namespace SDK
{

SwTimer::SwTimer()
    : mActive(false)
    , mComplete(false)
    , mInterval(0xFFFFFFFF)
    , mPrevSysTimer(0)
{}

/**
 * @brief Construct and start timer with given interval.
 * @param interval Interval in milliseconds.
 */
SwTimer::SwTimer(uint32_t interval)
    : mActive(true)
    , mComplete(false)
    , mInterval(interval)
    , mPrevSysTimer(getTicks())
{}

/**
 * @brief Enable or disable the timer.
 * @param value True to activate, false to deactivate.
 */
void SwTimer::setActive(bool value)
{
    mActive = value;
}

/**
 * @brief Check whether timer is active.
 * @return True if active, false otherwise.
 */
bool SwTimer::getActive(void)
{
    return mActive;
}

/**
 * @brief Set the interval duration.
 * @param value Interval in milliseconds.
 */
void SwTimer::setInterval(uint32_t value)
{
    mInterval = value;
}

/**
 * @brief Get the configured interval.
 * @return Interval in milliseconds.
 */
uint32_t SwTimer::getInterval(void)
{
    return mInterval;
}

/**
 * @brief Start or restart the timer with the current interval.
 */
void SwTimer::start(void)
{
    mPrevSysTimer = getTicks();
    mActive       = true;
}

/**
 * @brief Start timer with a new interval.
 * @param value Interval in milliseconds.
 */
void SwTimer::start(uint32_t value)
{
    setInterval(value);
    start();
}

/**
 * @brief Stop the timer.
 */
void SwTimer::stop(void)
{
    setActive(false);
}

/**
 * @brief Reset the anchor time but keep active state unchanged.
 */
void SwTimer::reset(void)
{
    mPrevSysTimer = getTicks();
}

/**
 * @brief Busy-wait for the specified number of milliseconds.
 * @param ms_number Duration in milliseconds.
 */
void SwTimer::wait(uint32_t ms_number)
{
    uint32_t ts = getTicks();

    while (passed(getTicks(), ts) < ms_number) {
        // spin
    }
}

/**
 * @brief Check if interval has elapsed since last tick.
 *
 * Advances the internal anchor on success.
 *
 * @return True if interval expired, false otherwise.
 */
bool SwTimer::check(void)
{
    if (!mActive) {
        return false;
    }

    uint32_t now = getTicks();
    bool res = (passed(now, mPrevSysTimer) >= mInterval);
    if (res) {
        mPrevSysTimer = now;
    }
    return res;
}

/**
 * @brief Check if interval has expired without modifying anchor.
 * @return True if expired, false otherwise.
 */
bool SwTimer::expired(void)
{
    if (!mActive) {
        return false;
    }
    return (passed(getTicks(), mPrevSysTimer) >= mInterval);
}

/**
 * @brief Time remaining until interval completion.
 * @return Milliseconds left, 0 if already expired or inactive.
 */
uint32_t SwTimer::left(void)
{
    if (!mActive) {
        return 0;
    }

    uint32_t passedMS = passed(getTicks(), mPrevSysTimer);

    return (passedMS >= mInterval) ? 0 : (mInterval - passedMS);
}

/**
 * @brief Calculate elapsed time handling overflow.
 * @param now Current tick value.
 * @param time_stamp Reference tick value.
 * @return Elapsed milliseconds.
 */
uint32_t SwTimer::passed(uint32_t now, uint32_t time_stamp)
{
    uint32_t result = 0xFFFFFFFF;

    if (now >= time_stamp) {
        result = now - time_stamp;
    } else {
        result = (result - time_stamp) + now + 1;
    }

    return result;
}

/**
 * @brief Elapsed time since a timestamp.
 * @param time_stamp Reference tick value.
 * @return Elapsed milliseconds.
 */
uint32_t SwTimer::passed(uint32_t time_stamp)
{
    return passed(getTicks(), time_stamp);
}

/**
 * @brief Elapsed time since the last reset/start.
 * @return Elapsed milliseconds.
 */
uint32_t SwTimer::passed(void)
{
    return passed(mPrevSysTimer);
}

} // namespace SDK
