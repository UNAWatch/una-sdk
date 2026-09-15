/**
 ******************************************************************************
 * @file    TrackIntervalsAlertScreen.cpp
 * @brief   Phase-change alert (see TrackIntervalsAlertScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackIntervalsAlertScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

using namespace SDK::GUI;

namespace
{
constexpr uint32_t kDismissMs = 5000;
} // namespace

TrackIntervalsAlertScreen::TrackIntervalsAlertScreen(Model& model)
    : Screen(model)
{
}

TrackIntervalsAlertScreen::~TrackIntervalsAlertScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackIntervalsAlertScreen::build()
{
    Theme::image(mRoot, &img_runningman_46x46, 97, 166);
    mRepeats = Theme::label(mRoot, Theme::Font::SemiBold35, "", 40, 115, 160);
    mTimer   = std::make_unique<Widgets::IntervalsTimer>(mRoot, 25, 41);
    mTimer->setLineVisible(false);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "");
}

void TrackIntervalsAlertScreen::onShow()
{
    // Use the snapshot carried by the alert message, not the live track data,
    // which may not have caught up with the new phase yet.
    const Track::IntervalsData& iv = mModel.getPendingAlertIntervals();

    const char* title = "WARM UP";
    uint32_t    color = Color::WHITE;
    switch (iv.phase) {
        case Track::IntervalsPhase::RUN:       title = "RUN";       color = Color::CYAN;        break;
        case Track::IntervalsPhase::REST:      title = "REST";      color = Color::YELLOW_DARK; break;
        case Track::IntervalsPhase::COOL_DOWN: title = "COOL DOWN"; break;
        default: break;
    }
    mTitle->setText(title);
    mTimer->setColor(color);

    if (iv.metric == Track::IntervalsMetric::DISTANCE) {
        const bool imperial = mModel.isUnitsImperial();
        mTimer->setPhaseDistance(Fmt::distUnits(iv.distRemaining, imperial), imperial);
        mTimer->setDescriptionVisible(false);
    } else if (iv.metric == Track::IntervalsMetric::TIME_OPEN) {
        mTimer->setOpen();
    } else {
        mTimer->setRemainingTime(iv.phaseTimerSec);
    }

    // Repeat counter only during RUN / REST; "n" alone for open-ended repeats.
    const bool showRepeats = iv.phase == Track::IntervalsPhase::RUN || iv.phase == Track::IntervalsPhase::REST;
    if (showRepeats) {
        if (iv.totalRepeats == 0) {
            lv_label_set_text_fmt(mRepeats, "%u", static_cast<unsigned>(iv.repeat));
        } else {
            lv_label_set_text_fmt(mRepeats, "%u/%u", static_cast<unsigned>(iv.repeat),
                                  static_cast<unsigned>(iv.totalRepeats));
        }
        lv_obj_remove_flag(mRepeats, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mRepeats, LV_OBJ_FLAG_HIDDEN);
    }

    mDismiss = lv_timer_create(&TrackIntervalsAlertScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackIntervalsAlertScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackIntervalsAlertScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::Track);
}
