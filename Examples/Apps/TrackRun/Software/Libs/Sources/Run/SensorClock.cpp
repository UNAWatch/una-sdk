/*******************************************************************************
 * @file   SensorClock.cpp
 * @brief  Run: "now" on the GPS sensor clock (SensorClock.hpp).
 ******************************************************************************/

#include "Run/SensorClock.hpp"

namespace Run {

int32_t SensorClock::onSample(uint32_t sensorMs, uint32_t sysMs) noexcept
{
    const int32_t obs = static_cast<int32_t>(sensorMs - sysMs);
    if (!mSynced) {
        const int32_t jump = obs - mOffset;
        mSynced  = true;
        mOffset  = obs;
        mSlewSys = sysMs;
        // Times handed out before were on the system clock: start again from
        // the sensor clock.
        mHaveOut = false;
        return jump;
    }
    // Offsets are modulo 2^32: compared by unsigned difference (a signed one
    // can overflow).
    if (static_cast<int32_t>(static_cast<uint32_t>(obs) - static_cast<uint32_t>(mOffset)) > 0) {
        // Less latency than any reading so far: the better offset, at once.
        // now() moves forward, never back.
        mOffset  = obs;
        mSlewSys = sysMs;
        return 0;
    }
    // A smaller offset: the clocks may be drifting apart. Follow it slowly.
    const int32_t  gap     = static_cast<int32_t>(static_cast<uint32_t>(mOffset) - static_cast<uint32_t>(obs));
    const uint32_t elapsed = sysMs - mSlewSys;
    const uint32_t allow   = (elapsed / 1000u) * kSlewMsPerS;
    if (allow > 0u) {
        const int32_t step = static_cast<int32_t>(allow) < gap ? static_cast<int32_t>(allow) : gap;
        mOffset -= step;
        mSlewSys = sysMs;
    }
    return 0;
}

uint32_t SensorClock::now(uint32_t sysMs) noexcept
{
    const uint32_t t = sysMs + static_cast<uint32_t>(mOffset);
    if (mHaveOut && static_cast<int32_t>(t - mLastOut) < 0) {
        return mLastOut;
    }
    mHaveOut = true;
    mLastOut = t;
    return t;
}

} // namespace Run
