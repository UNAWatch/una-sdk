/**
 ******************************************************************************
 * @file    SwTimer.hpp
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


#ifndef __SOFTWARE_TIMER_HPP__
#define __SOFTWARE_TIMER_HPP__

#include <stdint.h>

namespace SDK {
    
class SwTimer
{
public:
    SwTimer();
    SwTimer(uint32_t interval);

    /**
     * @brief Convert seconds to milliseconds.
     * @param v Number of seconds.
     * @return Equivalent duration in milliseconds.
     */
    static constexpr uint32_t seconds(uint32_t v) noexcept
    {
        return 1000UL * v;
    }

    /**
     * @brief Convert minutes to milliseconds.
     * @param v Number of minutes.
     * @return Equivalent duration in milliseconds.
     */
    static constexpr uint32_t minutes(uint32_t v) noexcept
    {
        return seconds(60UL) * v;
    }

    void setActive(bool value);
    bool getActive(void);

    void     setInterval(uint32_t value);
    uint32_t getInterval(void );

    void start(void);
    void start(uint32_t value);
    void stop(void);
    void reset(void);

    bool     check(void);
    bool     expired(void);
    uint32_t left(void);
    uint32_t passed(void);
    
    static uint32_t getTicks(void);
    static uint32_t passed(uint32_t now, uint32_t time_stamp);
    static uint32_t passed(uint32_t time_stamp);
    static void     wait(uint32_t ms_number);
    
private:
    bool     mActive;
    bool     mComplete;
    uint32_t mInterval;
    uint32_t mPrevSysTimer;
};

}

#endif
