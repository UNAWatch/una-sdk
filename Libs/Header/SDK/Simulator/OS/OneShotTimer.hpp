/******************************************************************************
 * @file    OneShortTimer.hpp
 * @date    31-March-2026
 * @author  Vlad Andriaysh
 * @brief   OneShortTimer header file
 * 
 ******************************************************************************
 */
 
#ifndef __ONE_SHORT_TIMER_HPP
#define __ONE_SHORT_TIMER_HPP
 
#include <functional>
#include <chrono>
#include <vector>
#include <thread>

namespace OS 
{
    class OneShotTimer
    {
    public:
        using Clock = std::chrono::steady_clock;
        using Callback = std::function<void()>;
        using TimerId = uint32_t;

        /**
         * @brief   Get OneShotTimer instance.
         * @retval  OneShotTimer instance.
         */
        static OneShotTimer& getInstance()
        {
            static OneShotTimer sInstance;
            return sInstance;
        }

        /**
         * @brief   Start a one-shot timer.
         * @param   delayMs: timer delay in milliseconds before callback execution.
         * @param   cb: callback function that will be invoked when the timer expires.
         * @retval  Timer identifier used to control the timer.
         */
        TimerId start(uint32_t delayMs, Callback cb);

        /**
         * @brief   Stop an active timer.
         * @param   id: identifier of the timer returned by start().
         */
        void stop(TimerId id);

        /**
         * @brief   Check whether the timer is active.
         * @param   id: identifier of the timer returned by start().
         * @retval  true if the timer is active, false otherwise.
         */
        bool isActive(TimerId id);

    private:

        OneShotTimer();
        ~OneShotTimer();

        OneShotTimer(const OneShotTimer&) = delete;
        OneShotTimer& operator=(const OneShotTimer&) = delete;

        void run();

        struct Timer
        {
            TimerId id;
            Clock::time_point fireTime;
            Callback callback;
            bool active;
        };

        std::thread mThread;
        std::vector<Timer> mTimers;
        TimerId mNextId = 1;
    };
}

#endif //__ONE_SHORT_TIMER_HPP