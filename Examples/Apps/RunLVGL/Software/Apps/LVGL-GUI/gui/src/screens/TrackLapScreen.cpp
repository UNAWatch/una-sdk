/**
 ******************************************************************************
 * @file    TrackLapScreen.cpp
 * @brief   Lap popup (see TrackLapScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackLapScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"

namespace
{
constexpr uint32_t kDismissMs = 5000;
} // namespace

TrackLapScreen::TrackLapScreen(Model& model)
    : Screen(model)
{
}

TrackLapScreen::~TrackLapScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackLapScreen::build()
{
    using F = Theme::Font;
    Theme::label(mRoot, F::Italic18, "Dist.", 18, 58, 100);
    mDistance = Theme::label(mRoot, F::SemiBold35, Strings::kNoValue, 18, 82, 100);
    Theme::vline(mRoot, 119, 84, 44);
    Theme::label(mRoot, F::Italic18, "Time", 122, 58, 100);
    mTime = Theme::label(mRoot, F::SemiBold35, "0:00", 122, 82, 100);
    Theme::hline(mRoot, 35, 127, 170);
    Theme::label(mRoot, F::Italic18, "Avg. Pace", 70, 137, 100);
    // Full width: a slow pace such as "22:00" is wider than the design's 100 px box.
    mPace = Theme::label(mRoot, F::SemiBold40, Strings::kNoValue, 0, 158, 240);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "Lap");
}

void TrackLapScreen::onShow()
{
    mModel.resetIdleTimer();

    const bool imperial   = mModel.isUnitsImperial();
    const Track::Data& d  = mModel.getTrackData();
    char buf[16];

    snprintf(buf, sizeof(buf), "Lap %lu", static_cast<unsigned long>(d.lapNum + 1));
    mTitle->setText(buf);
    Fmt::distanceLap(buf, sizeof(buf), Fmt::distUnits(d.lapDistance, imperial));
    lv_label_set_text(mDistance, buf);
    Fmt::shortTime(buf, sizeof(buf), d.lapTime);
    lv_label_set_text(mTime, buf);
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(d.lapPace, imperial));
    lv_label_set_text(mPace, buf);

    mDismiss = lv_timer_create(&TrackLapScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackLapScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackLapScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::Track);
}
