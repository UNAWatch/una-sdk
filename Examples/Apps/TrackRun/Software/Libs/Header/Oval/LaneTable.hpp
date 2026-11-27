/*******************************************************************************
 * @file   LaneTable.hpp
 * @brief  Oval: lap length of every lane, computed from the track geometry.
 *
 *         Lanes 1 to 8: lane 1 is measured 0.30 m out from the kerb, every
 *         other lane 0.20 m out from its inner line, lanes are 1.22 m wide,
 *         and lane 1 is 400 m. Only the bends differ between lanes, so lane
 *         n is 400 + 2 pi (offset n - offset 1), whatever the bend radius of
 *         the track.
 ******************************************************************************/

#ifndef OVAL_LANE_TABLE_HPP
#define OVAL_LANE_TABLE_HPP

#include <cstdint>

namespace Oval {

class LaneTable {
public:
    static constexpr uint8_t kMaxLanes     = 8;
    static constexpr uint8_t kOutdoorLanes = 8;

    static constexpr float kOutdoorLane1M     = 400.0f;
    static constexpr float kOutdoorLaneWidthM = 1.22f;
    static constexpr float kLane1LineM        = 0.30f; ///< Lane 1 line, out from the kerb
    static constexpr float kLineFromInnerM    = 0.20f; ///< Other lanes, out from their inner line

    LaneTable() noexcept;

    static LaneTable outdoor() noexcept;

    uint8_t laneCount() const noexcept { return mCount; }
    bool    isValidLane(uint8_t lane) const noexcept { return lane >= 1 && lane <= mCount; }

    /** Lap length of a lane (1-based) in metres, or 0 for an invalid lane. */
    float lapLengthM(uint8_t lane) const noexcept;

    /** Lateral offset of a lane's measurement line from the kerb, in metres,
     *  or 0 for an invalid lane. Calibration uses it. */
    float lineOffsetM(uint8_t lane) const noexcept;

private:
    uint8_t mCount  = 0;
    float   mLengthM[kMaxLanes] = {};
    float   mOffsetM[kMaxLanes] = {};
};

} // namespace Oval

#endif // OVAL_LANE_TABLE_HPP
