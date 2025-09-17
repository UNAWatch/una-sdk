#ifndef __SwTimer_hpp__
#define __SwTimer_hpp__

#include <stdint.h>
#include "SDK/Interfaces/IKernel.hpp"

namespace SDK {
    
#define SW_TIMER_SECONDS(v)     (1000UL * (v))
#define SW_TIMER_MINUTES(v)     (SW_TIMER_SECONDS(1) * 60 * (v))

extern "C" {
    extern const ::IKernel* kernel;

    static uint32_t getSwTimerTicks(void)
    {
        return kernel->app.getTimeMs();
    }
}

class SwTimer
{
public:
    SwTimer()                            {m_Active = false; m_Complete = false; m_Interval = 0xFFFFFFFF; m_PrevSysTimer = 0; };
    SwTimer(uint32_t interval)           {m_Active = true;  m_Complete = false; m_Interval = interval;   m_PrevSysTimer = getSwTimerTicks(); };

    void setActive(bool value)           { m_Active = value; }
    bool getActive(void)                 { return m_Active;  }

    void     setInterval(uint32_t value) { m_Interval = value; }
    uint32_t getInterval(void )          { return m_Interval;  }

    void start(void)                     { m_PrevSysTimer = getSwTimerTicks(); m_Active = true; }
    void start(uint32_t value)           { setInterval(value); start(); }

    void stop(void)                      { setActive(false); }

    void reset(void)                     { m_PrevSysTimer = getSwTimerTicks(); }

    static void wait(uint32_t ms_number)
    {
        uint32_t ts = getSwTimerTicks();

        while (passed(getSwTimerTicks(), ts) < ms_number) {
        }
    }

    bool check(void)
    {
        uint32_t now;
        bool res;

        if (m_Active == false)
            return false;

        now = getSwTimerTicks();

        res = (passed(now, m_PrevSysTimer) >= m_Interval);
        if (res)
            m_PrevSysTimer = now;

        return res;
    };
    
    bool expired(void)
    {
        if (m_Active == false) {
            return false;
        }

        return (passed(getSwTimerTicks(), m_PrevSysTimer) >= m_Interval);
    };

    uint32_t left(void)
    {
        if (m_Active == false) {
            return 0;
        }

        uint32_t passedMS = passed(getSwTimerTicks(), m_PrevSysTimer);

        if (passedMS >= m_Interval) {
            return 0;
        }

        return m_Interval - passedMS;
    };

    static uint32_t passed(uint32_t now, uint32_t time_stamp)
    {
        uint32_t result = (uint32_t)(-1);

        if (now >= time_stamp)
            result = now - time_stamp;
        else
            result = (result - time_stamp) + now + 1;

        return result;
    }

    static uint32_t passed(uint32_t time_stamp)
    {
        return passed(getSwTimerTicks(), time_stamp);
    }

    uint32_t passed(void)
    {
        return passed(m_PrevSysTimer);
    }
    
private:
    bool     m_Active;
    bool     m_Complete;
    uint32_t m_Interval;
    uint32_t m_PrevSysTimer;
};

}

#endif
