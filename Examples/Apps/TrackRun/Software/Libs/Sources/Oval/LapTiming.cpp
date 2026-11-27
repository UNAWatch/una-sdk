/*******************************************************************************
 * @file   LapTiming.cpp
 * @brief  Oval: line-fitted crossing times. See LapTiming.hpp.
 ******************************************************************************/

#include "Oval/LapTiming.hpp"

#include "Oval/Sample.hpp"

#include <cmath>

namespace Oval {

namespace {

constexpr float kMinSlopeMps = 0.5f; // slower than this, the fitted time means little

} // namespace

void LapTiming::push(const Entry& e) noexcept
{
    mE[mHead] = e;
    mHead = static_cast<uint8_t>((mHead + 1) % kSlots);
    if (mCount < kSlots) {
        ++mCount;
    }
}

const LapTiming::Entry& LapTiming::at(uint8_t i) const noexcept
{
    const uint8_t oldest = static_cast<uint8_t>((mHead + kSlots - mCount) % kSlots);
    return mE[(oldest + i) % kSlots];
}

bool LapTiming::fitLine(uint32_t tc, uint32_t beforeMs, uint32_t afterMs, uint8_t flag, Vec2 c, Vec2 dir,
                        float target, uint32_t& outMs) const noexcept
{
    float   st = 0.0f, sy = 0.0f, stt = 0.0f, sty = 0.0f;
    uint8_t n = 0, nBefore = 0, nAfter = 0;
    for (uint8_t i = 0; i < mCount; ++i) {
        const Entry&  e  = at(i);
        const int32_t dt = msDiff(e.tMs, tc);
        if (!(e.flags & flag) || dt < -static_cast<int32_t>(beforeMs) || dt > static_cast<int32_t>(afterMs)) {
            continue;
        }
        const float t = 0.001f * static_cast<float>(dt);
        const float y = (flag == kPosition) ? dot(e.p - c, dir) : e.total;
        st  += t;
        sy  += y;
        stt += t * t;
        sty += t * y;
        ++n;
        if (dt <= 0) {
            ++nBefore;
        } else {
            ++nAfter;
        }
    }
    if (n < 3 || nBefore == 0 || nAfter == 0) {
        return false;
    }
    const float fn  = static_cast<float>(n);
    const float sxx = stt - st * st / fn;
    if (!(sxx > 1e-6f)) {
        return false;
    }
    const float b = (sty - st * sy / fn) / sxx;
    if (!(b > kMinSlopeMps)) {
        return false;
    }
    const float a = (sy - b * st) / fn;
    const float t = (target - a) / b;
    // The fit must land inside the window it was fitted on.
    if (!(t >= -0.001f * static_cast<float>(beforeMs)) || !(t <= 0.001f * static_cast<float>(afterMs))) {
        return false;
    }
    outMs = tc + static_cast<uint32_t>(static_cast<int32_t>(std::lround(t * 1000.0f)));
    return true;
}

bool LapTiming::fitGateCrossing(uint32_t tc, Vec2 c, Vec2 dir, uint32_t beforeMs, uint32_t afterMs,
                                uint32_t& outMs) const noexcept
{
    return fitLine(tc, beforeMs, afterMs, kPosition, c, dir, 0.0f, outMs);
}

bool LapTiming::fitTotalCrossing(uint32_t tc, float target, uint32_t beforeMs, uint32_t afterMs,
                                 uint32_t& outMs) const noexcept
{
    return fitLine(tc, beforeMs, afterMs, kCalibrated, Vec2(), Vec2(), target, outMs);
}

bool LapTiming::timeOfTotal(float target, uint32_t beforeMs, uint32_t afterMs, uint32_t& outMs) const noexcept
{
    if (mCount < 2) {
        return false;
    }
    for (uint8_t i = 1; i < mCount; ++i) {
        const Entry& a = at(static_cast<uint8_t>(i - 1));
        const Entry& b = at(i);
        const uint8_t modes = kCalibrated | kCounting;
        if (!(a.flags & modes) || !(b.flags & modes)) {
            continue;
        }
        if (i == 1 && a.total >= target) {
            return false; // reached before the oldest reading kept
        }
        if (!(a.total < target && b.total >= target)) {
            continue;
        }
        const float    f  = (target - a.total) / (b.total - a.total);
        const int32_t  dt = msDiff(b.tMs, a.tMs);
        const uint32_t tc = a.tMs + static_cast<uint32_t>(std::lround(f * static_cast<float>(dt > 0 ? dt : 0)));
        if ((a.flags & kCalibrated) && (b.flags & kCalibrated)
            && fitTotalCrossing(tc, target, beforeMs, afterMs, outMs)) {
            return true;
        }
        outMs = tc;
        return true;
    }
    return false;
}

} // namespace Oval
