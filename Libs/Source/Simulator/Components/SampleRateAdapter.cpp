/**
 ******************************************************************************
 * @file    SampleRateAdapter.cpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sample rate adapter
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "SRA"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/SampleRateAdapter.hpp"

//#include "SwTimer.hpp"

namespace Sensor
{

SampleRateAdapter::SampleRateAdapter(uint64_t expectedPeriod)
    : mExpectedPeriod(expectedPeriod)
    //, mLastEmitedTimestamp(Driver::SwTimer::getTicks())
    , mLastEmitedTimestamp(0)
    , mLastSampleTimestamp(0)
{
}

void SampleRateAdapter::setExpectedPeriod(uint64_t us)
{
    mExpectedPeriod = us;
}

void SampleRateAdapter::reset()
{
    mLastSampleTimestamp = 0;
}

bool SampleRateAdapter::shouldEmit()
{
    //return shouldEmit(Driver::SwTimer::getTicks());
    return shouldEmit(0);
}

static uint64_t passed(uint64_t now, uint64_t time_stamp)
{
    uint64_t result = (uint64_t)(-1);

    if (now >= time_stamp)
        result = now - time_stamp;
    else
        result = (result - time_stamp) + now + 1;

    return result;
}

bool SampleRateAdapter::shouldEmit(uint64_t now)
{
    const uint64_t samplePeriod = passed(now, mLastSampleTimestamp);
    const uint64_t updatePeriod = passed(now, mLastEmitedTimestamp);

    const bool emit = (updatePeriod >= mExpectedPeriod) ||
                      ((updatePeriod + samplePeriod) > mExpectedPeriod);

    mLastSampleTimestamp = now;

    if (!emit) {
        LOG_DEBUG("-\n");
        return false;
    }

    mLastEmitedTimestamp = now;

    LOG_DEBUG("EP=%u UP=%u SP=%u\n", mExpectedPeriod, updatePeriod, samplePeriod);

    return true;
}

} // namespace Sensor
