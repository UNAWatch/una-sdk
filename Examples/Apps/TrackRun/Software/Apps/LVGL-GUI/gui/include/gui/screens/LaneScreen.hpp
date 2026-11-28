/**
 ******************************************************************************
 * @file    LaneScreen.hpp
 * @brief   The lane picker: "No lane" (the road), then Lane 1 to Lane 8.
 *
 * Opened from the start screen (the lane Start uses), the pause menu, or a
 * hold of Lap in a run (Settings > Lane shortcut), and goes back there. In
 * a run the lane applies from the moment it is chosen. From a hold of Lap
 * the picker goes back by itself after kFromRunIdleMs without a press, so a
 * hold by mistake does not leave the run screen hidden.
 ******************************************************************************
 */

#ifndef LANE_SCREEN_HPP
#define LANE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/LaneWheel.hpp"
#include "gui/widgets/Widgets.hpp"

class LaneScreen : public Screen
{
public:
    static constexpr uint32_t kFromRunIdleMs = 8000;

    explicit LaneScreen(Model& model);
    ~LaneScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    static void idleCb(lv_timer_t* t);
    void back();

    std::unique_ptr<LaneWheel>        mLanes;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    lv_timer_t*                       mIdle = nullptr;
};

#endif // LANE_SCREEN_HPP
