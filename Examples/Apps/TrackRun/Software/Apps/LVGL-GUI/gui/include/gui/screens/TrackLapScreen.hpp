/**
 ******************************************************************************
 * @file    TrackLapScreen.hpp
 * @brief   Lap popup: number, distance, time and average pace of the lap just
 *          ended, shown for five seconds, then back to the track faces, or to
 *          the pause menu for a lap taken there.
 ******************************************************************************
 */

#ifndef TRACK_LAP_SCREEN_HPP
#define TRACK_LAP_SCREEN_HPP

#include <memory>

#include "gui/screens/LaneHold.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackLapScreen : public Screen
{
public:
    explicit TrackLapScreen(Model& model);
    ~TrackLapScreen() override;

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    /// The lap the screen opened for (lap button) has come back, or another
    /// ended: show it, for the full time again.
    void onLapChanged(uint8_t lapEnd) override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);
    void showLap();

    lv_timer_t* mDismiss = nullptr;
    LaneHold    mLaneHold;
    lv_obj_t* mDistance = nullptr;
    lv_obj_t* mTime     = nullptr;
    lv_obj_t* mPace     = nullptr;
    lv_obj_t* mChip     = nullptr;   ///< Against the lap before: green faster, coral slower, white even
    lv_obj_t* mChipText = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_LAP_SCREEN_HPP
