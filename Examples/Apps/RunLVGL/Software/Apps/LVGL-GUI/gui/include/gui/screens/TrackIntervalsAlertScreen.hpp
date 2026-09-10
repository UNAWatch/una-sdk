/**
 ******************************************************************************
 * @file    TrackIntervalsAlertScreen.hpp
 * @brief   Phase-change alert: the new phase's name, its target (time,
 *          distance or Open) and the repeat counter, shown for five seconds.
 ******************************************************************************
 */

#ifndef TRACK_INTERVALS_ALERT_SCREEN_HPP
#define TRACK_INTERVALS_ALERT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackIntervalsAlertScreen : public Screen
{
public:
    explicit TrackIntervalsAlertScreen(Model& model);
    ~TrackIntervalsAlertScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mRepeats = nullptr;
    std::unique_ptr<Widgets::Title>          mTitle;
    std::unique_ptr<Widgets::IntervalsTimer> mTimer;
};

#endif // TRACK_INTERVALS_ALERT_SCREEN_HPP
