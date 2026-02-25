/**
 ******************************************************************************
 * @file    Timer.сpp
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

#include "SDK/Timer/Timer.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

#include <cstring>

namespace SDK {

Timer::Timer()
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mActive(false)
    , mInterval(0xFFFFFFFFu)
    , mPrevTimestamp(0u)
{}

Timer::Timer(uint32_t interval)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mActive(false)
    , mInterval(interval)
    , mPrevTimestamp(0u)
{}

void Timer::setActive(bool value)
{
    mActive = value;
}

bool Timer::isActive(void) const
{
    return mActive;
}

void Timer::setInterval(uint32_t value)
{
    mInterval = value;
}

uint32_t Timer::getInterval(void) const
{
    return mInterval;
}

void Timer::start(void)
{
    mPrevTimestamp = getTimestamp();
    mActive = true;
}

void Timer::start(uint32_t value)
{
    setInterval(value);
    start();
}

void Timer::stop(void)
{
    setActive(false);
}

void Timer::reset(void)
{
    mPrevTimestamp = getTimestamp();
}

bool Timer::tick(void)
{
    if (!mActive) {
        return false;
    }

    const uint32_t now = getTimestamp();

    if (elapsed(now, mPrevTimestamp) < mInterval) {
        return false;
    }

    mPrevTimestamp = now;

    return true;
}

bool Timer::expired(void) const
{
    if (!mActive) {
        return false;
    }

    return (elapsed(getTimestamp(), mPrevTimestamp) >= mInterval);
}

uint32_t Timer::left(void) const
{
    if (!mActive) {
        return 0u;
    }

    const uint32_t interval = elapsed(getTimestamp(), mPrevTimestamp);

    if (interval >= mInterval) {
        return 0u;
    }

    return (mInterval - interval);
}

uint32_t Timer::getTimestamp(void) const
{
    return mKernel.sys.getTimeMs();
}

uint32_t Timer::elapsed(uint32_t now, uint32_t timestamp)
{
    return (now >= timestamp) ? now - timestamp : (0xFFFFFFFF - timestamp) + now + 1;
}

uint32_t Timer::elapsed(void) const
{
    return elapsed(getTimestamp(), mPrevTimestamp);
}

} // namespace SDK
