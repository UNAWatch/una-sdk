/******************************************************************************
 * @file    OneShortTimer.cpp
 * @date    31-March-2026
 * @author  Vlad Andriaysh
 * @brief   OneShortTimer source file
 * 
 ******************************************************************************
 */

#include "SDK/Simulator/OS/OneShotTimer.hpp"
#include <SDK/Simulator/Kernel/Mock/System.hpp>

using namespace OS;

OneShotTimer::OneShotTimer()
{
    mThread = std::thread(&OneShotTimer::run, this);
}
OneShotTimer::~OneShotTimer()
{
    if (mThread.joinable())
        mThread.join();
}

OneShotTimer::TimerId OneShotTimer::start(uint32_t delayMs, Callback cb)
{
    Timer timer;
    timer.id = mNextId++;
    timer.fireTime = Clock::now() + std::chrono::milliseconds(delayMs);
    timer.callback = cb;
    timer.active = true;

    mTimers.push_back(timer);

    return timer.id;
}

void OneShotTimer::stop(TimerId id)
{
    for (auto& t : mTimers)
    {
        if (t.id == id)
        {
            t.active = false;
            return;
        }
    }
}

bool OneShotTimer::isActive(TimerId id)
{
    for (auto& t : mTimers)
    {
        if (t.id == id)
        {
            return t.active;
        }
    }
    return false;
}

void OneShotTimer::run()
{
    while (1) {
        if (!SDK::Simulator::Mock::SystemGUI::isAppRunning()) {
            return;
        }

        OS::Delay(50);

        auto now = Clock::now();

        for (size_t i = 0; i < mTimers.size(); )
        {
            auto& t = mTimers[i];

            if (!t.active)
            {
                mTimers.erase(mTimers.begin() + i);
                continue;
            }

            if (now >= t.fireTime)
            {
                t.callback();
                mTimers.erase(mTimers.begin() + i);
            }
            else
            {
                ++i;
            }
        }
    }
}