/**
 ******************************************************************************
 * @file    LaneWheel.hpp
 * @brief   Track Run's lane picker: the SDK wheel menu with "Lane 1" to
 *          "Lane 8", and "No lane" (the road) above lane 1; not
 *          circular, so it stops at either end. Up (L1) moves to the item
 *          above, Down (L2) to the one below.
 *
 * Used by the pre-run screen and the lane picker during a run (the pause
 * menu's "Change lane", or a hold of Lap).
 *
 * The wheel is the SDK's, as RunLVGL's menus. The lane badge
 * sits on the lens beside the selected lane (the SDK wheel has no badge of
 * its own, so the selected item leaves room for it and the badge follows the
 * slide's midpoint), and the track status is one white line under the
 * screen's title, where RunLVGL's workout list writes its group.
 ******************************************************************************
 */

#ifndef LANE_WHEEL_HPP
#define LANE_WHEEL_HPP

#include <cstdint>
#include <memory>

#include "Settings.hpp"
#include "gui/widgets/Kit.hpp"
#include "gui/widgets/WheelMenu.hpp"

class LaneWheel
{
public:
    /// noLane: "No lane" (lane 0) as the first item.
    LaneWheel(lv_obj_t* parent, uint8_t count, bool noLane);

    void    select(uint8_t lane);   ///< 0: "No lane" (lane 1 without it)
    uint8_t lane() const;           ///< 0: "No lane"
    void    up();     ///< the item above (L1)
    void    down();   ///< the item below (L2)

    /// The track status under the title, in white; nullptr or "" shows none.
    void setStatus(const char* text);

    WheelMenu& menu() { return *mMenu; }

private:
    static constexpr uint8_t kLanes = Settings::kLanes;

    static void slideMid(void* ctx, uint16_t target);
    void        showBadge(uint8_t lane);

    uint8_t                         mCount  = kLanes;
    bool                            mNoLane = false;
    WheelMenu::Item                 mItems[kLanes + 1] {};
    char                            mTexts[kLanes][8] {};
    std::unique_ptr<WheelMenu>      mMenu;
    std::unique_ptr<Kit::LaneBadge> mBadge;
    lv_obj_t*                       mStatus = nullptr;
};

#endif // LANE_WHEEL_HPP
