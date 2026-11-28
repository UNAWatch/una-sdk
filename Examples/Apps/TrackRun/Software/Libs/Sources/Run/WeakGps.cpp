/*******************************************************************************
 * @file   WeakGps.cpp
 * @brief  Run: the weak GPS flag (WeakGps.hpp).
 ******************************************************************************/

#include "Run/WeakGps.hpp"

namespace Run {

void WeakGps::add(uint32_t tMs, bool fix, float hdop, float maxHdop) noexcept
{
    if (!mHaveAny) {
        mHaveAny = true;
        mFirstT  = tMs;
    }
    if (fix && !(hdop > maxHdop)) {
        mHaveGood = true;
        mGoodT    = tMs;
    }
}

bool WeakGps::weak(uint32_t nowMs) const noexcept
{
    if (!mHaveAny) {
        return false;
    }
    const uint32_t since = mHaveGood ? mGoodT : mFirstT;
    return static_cast<int32_t>(nowMs - since) > static_cast<int32_t>(kWeakAfterMs);
}

} // namespace Run
