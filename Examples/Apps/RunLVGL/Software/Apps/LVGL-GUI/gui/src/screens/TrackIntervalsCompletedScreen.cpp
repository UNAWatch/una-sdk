/**
 ******************************************************************************
 * @file    TrackIntervalsCompletedScreen.cpp
 * @brief   "Workout Completed" notice (see the header).
 ******************************************************************************
 */

#include "gui/screens/TrackIntervalsCompletedScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

namespace
{
constexpr uint32_t kDismissMs = 5000;
} // namespace

TrackIntervalsCompletedScreen::TrackIntervalsCompletedScreen(Model& model)
    : Screen(model)
{
}

TrackIntervalsCompletedScreen::~TrackIntervalsCompletedScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackIntervalsCompletedScreen::build()
{
    Theme::image(mRoot, &img_runningman_46x46, 97, 166);
    Theme::label(mRoot, Theme::Font::Regular18, "Completed", 0, 99, 240);
    Theme::label(mRoot, Theme::Font::SemiBold35, "Workout", 0, 55, 240);
}

void TrackIntervalsCompletedScreen::onShow()
{
    // Only a notice: the service has already left intervals mode and opened a
    // fresh lap, so the session continues on the normal track faces.
    mDismiss = lv_timer_create(&TrackIntervalsCompletedScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackIntervalsCompletedScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackIntervalsCompletedScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::Track);
}
