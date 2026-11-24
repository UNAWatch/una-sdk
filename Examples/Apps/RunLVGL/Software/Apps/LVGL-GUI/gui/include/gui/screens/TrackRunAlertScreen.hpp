/**
 ******************************************************************************
 * @file    TrackRunAlertScreen.hpp
 * @brief   Run alert pop-up: "Too fast", "Too slow", "HR high" or "HR low",
 *          with the pace or heart rate that set it off, shown for three
 *          seconds, then back to the track faces.
 ******************************************************************************
 */

#ifndef TRACK_RUN_ALERT_SCREEN_HPP
#define TRACK_RUN_ALERT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackRunAlertScreen : public Screen
{
public:
    explicit TrackRunAlertScreen(Model& model);
    ~TrackRunAlertScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mValue   = nullptr;
    lv_obj_t*   mUnit    = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_RUN_ALERT_SCREEN_HPP
