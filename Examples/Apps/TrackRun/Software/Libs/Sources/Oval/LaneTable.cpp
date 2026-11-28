/*******************************************************************************
 * @file   LaneTable.cpp
 * @brief  Oval: lane lap lengths from the track geometry.
 ******************************************************************************/

#include "Oval/LaneTable.hpp"

namespace Oval {

namespace {

constexpr double kTwoPi = 6.283185307179586;

} // namespace

LaneTable::LaneTable() noexcept
{
    mCount  = kOutdoorLanes;
    for (uint8_t i = 0; i < kOutdoorLanes; ++i) {
        // Lane 1 is measured 0.30 m from the kerb; lane n >= 2 is measured
        // 0.20 m out from its inner line, which is (n - 1) lane widths out.
        const double offset = (i == 0)
            ? static_cast<double>(kLane1LineM)
            : i * static_cast<double>(kOutdoorLaneWidthM) + static_cast<double>(kLineFromInnerM);
        const double length = static_cast<double>(kOutdoorLane1M)
            + kTwoPi * (offset - static_cast<double>(kLane1LineM));
        mOffsetM[i] = static_cast<float>(offset);
        mLengthM[i] = static_cast<float>(length);
    }
}

LaneTable LaneTable::outdoor() noexcept
{
    return LaneTable();
}

float LaneTable::lapLengthM(uint8_t lane) const noexcept
{
    return isValidLane(lane) ? mLengthM[lane - 1] : 0.0f;
}

float LaneTable::lineOffsetM(uint8_t lane) const noexcept
{
    return isValidLane(lane) ? mOffsetM[lane - 1] : 0.0f;
}

} // namespace Oval
