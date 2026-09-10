/**
 ******************************************************************************
 * @file    TrackIntervalsCompletedScreen.hpp
 * @brief   "Workout Completed" notice shown for five seconds when the
 *          programmed intervals finish; the session itself keeps running.
 ******************************************************************************
 */

#ifndef TRACK_INTERVALS_COMPLETED_SCREEN_HPP
#define TRACK_INTERVALS_COMPLETED_SCREEN_HPP

#include "gui/screens/Screen.hpp"

class TrackIntervalsCompletedScreen : public Screen
{
public:
    explicit TrackIntervalsCompletedScreen(Model& model);
    ~TrackIntervalsCompletedScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
};

#endif // TRACK_INTERVALS_COMPLETED_SCREEN_HPP
