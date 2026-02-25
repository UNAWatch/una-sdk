/**
 ******************************************************************************
 * @file    Timer.hpp
 * @date    25-February-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Lightweight non-blocking software timer based on Kernel time source
 *
 ******************************************************************************
 *
 * Timer provides millisecond-resolution timing using SDK::Kernel system time.
 * It supports 32-bit timestamp wrap-around and can be used in polling-based
 * state machines or periodic task execution without blocking.
 *
 ******************************************************************************
 */

#ifndef SDK_TIMER_HPP
#define SDK_TIMER_HPP

#include "SDK/Kernel/Kernel.hpp"
#include <cstdint>

namespace SDK {

/**
 * @brief Convert seconds to milliseconds.
 */
#define TIMER_SECONDS(v)    (1000UL * (v))

/**
 * @brief Convert minutes to milliseconds.
 */
#define TIMER_MINUTES(v)    (TIMER_SECONDS(1) * 60UL * (v))

/**
 * @brief Simple non-blocking software timer based on Kernel time source.
 *
 * Timer uses millisecond timestamp from @ref SDK::Kernel and supports
 * 32-bit wrap-around handling.
 *
 * Typical usage:
 * @code
 * SDK::Timer t(TIMER_SECONDS(1));
 * t.start();
 *
 * if (t.tick()) {
 *     // executed every 1 second
 * }
 * @endcode
 */
class Timer
{
public:
    /**
     * @brief Construct timer with default interval (0xFFFFFFFF).
     *
     * Timer is inactive after construction.
     */
    Timer();

    /**
     * @brief Construct timer with given interval in milliseconds.
     * @param interval Timer interval in milliseconds.
     *
     * Timer is inactive after construction.
     */
    explicit Timer(uint32_t interval);

    /**
     * @brief Enable or disable timer.
     * @param value true – active, false – inactive.
     */
    void setActive(bool value);

    /**
     * @brief Check whether timer is active.
     * @return true if active.
     */
    bool isActive(void) const;

    /**
     * @brief Set timer interval.
     * @param value Interval in milliseconds.
     */
    void setInterval(uint32_t value);

    /**
     * @brief Get timer interval.
     * @return Interval in milliseconds.
     */
    uint32_t getInterval(void) const;

    /**
     * @brief Start timer using current interval.
     *
     * Resets internal timestamp and activates timer.
     */
    void start(void);

    /**
     * @brief Set interval and start timer.
     * @param value Interval in milliseconds.
     */
    void start(uint32_t value);

    /**
     * @brief Stop timer.
     *
     * Timer becomes inactive.
     */
    void stop(void);

    /**
     * @brief Reset internal timestamp without changing active state.
     */
    void reset(void);

    /**
     * @brief Check whether interval elapsed and auto-reload timestamp.
     *
     * If interval has elapsed, internal timestamp is updated to current time.
     *
     * @return true if interval elapsed.
     */
    bool tick(void);

    /**
     * @brief Check whether interval elapsed.
     *
     * Does not modify internal timestamp.
     *
     * @return true if interval elapsed.
     */
    bool expired(void) const;

    /**
     * @brief Get remaining time until expiration.
     *
     * @return Remaining time in milliseconds.
     *         Returns 0 if inactive or already expired.
     */
    uint32_t left(void) const;

    /**
     * @brief Get current system timestamp in milliseconds.
     *
     * @return Current time from Kernel.
     */
    uint32_t getTimestamp(void) const;

    /**
     * @brief Calculate elapsed time between two timestamps.
     *
     * Handles 32-bit wrap-around.
     *
     * @param now Current timestamp.
     * @param timestamp Previous timestamp.
     * @return Elapsed time in milliseconds.
     */
    static uint32_t elapsed(uint32_t now, uint32_t timestamp);

    /**
     * @brief Calculate elapsed time from last start/reset.
     *
     * @return Elapsed time in milliseconds.
     */
    uint32_t elapsed(void) const;

private:
    const SDK::Kernel& mKernel;        ///< Reference to Kernel time provider.
    bool               mActive;        ///< Timer active state.
    uint32_t           mInterval;      ///< Interval in milliseconds.
    uint32_t           mPrevTimestamp; ///< Last reference timestamp.
};

} // namespace SDK

#endif // SDK_TIMER_HPP
