/**
 ******************************************************************************
 * @file    TrackLapScreen.hpp
 * @brief   Lap popup: number, distance, time and average pace of the lap just
 *          ended, shown for five seconds, then back to the track faces.
 ******************************************************************************
 */

#ifndef TRACK_LAP_SCREEN_HPP
#define TRACK_LAP_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackLapScreen : public Screen
{
public:
    explicit TrackLapScreen(Model& model);
    ~TrackLapScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
    lv_obj_t* mDistance = nullptr;
    lv_obj_t* mTime     = nullptr;
    lv_obj_t* mPace     = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_LAP_SCREEN_HPP
