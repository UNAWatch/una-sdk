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

#include "LapCompare.hpp"

namespace
{
constexpr uint32_t kDismissMs = 5000;

// The delta chip, under the pace, inside the round screen at its corners.
constexpr int32_t kChipX = 60;
constexpr int32_t kChipY = 201;
constexpr int32_t kChipW = 120;
constexpr int32_t kChipH = 22;
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
    Theme::label(mRoot, F::Italic18, "Avg. Pace", 70, 130, 100);
    // Full width: a slow pace such as "22:00" is wider than the design's 100 px box.
    mPace = Theme::label(mRoot, F::SemiBold40, Strings::kNoValue, 0, 150, 240);
    // Against the lap before (LapCompare): black words on green (faster),
    // coral (slower) or white (even); hidden with nothing to compare.
    mChip     = Theme::box(mRoot, kChipX, kChipY, kChipW, kChipH, SDK::GUI::Color::WHITE);
    mChipText = Theme::label(mRoot, F::Medium18, "", kChipX, kChipY, kChipW, LV_TEXT_ALIGN_CENTER,
                             SDK::GUI::Color::BLACK);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "Lap");
}

void TrackLapScreen::onShow()
{
    showLap();

    mDismiss = lv_timer_create(&TrackLapScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackLapScreen::onLapChanged(uint8_t /*lapEnd*/)
{
    showLap();
    if (mDismiss) {
        lv_timer_reset(mDismiss);
    }
}

void TrackLapScreen::showLap()
{
    mModel.takePopup(Model::Popup::Lap);
    mModel.resetIdleTimer();

    const bool imperial       = mModel.isUnitsImperial();
    const Model::LapInfo& lap = mModel.getLap();
    char buf[16];

    snprintf(buf, sizeof(buf), "Lap %lu", static_cast<unsigned long>(lap.num + 1));
    mTitle->setText(buf);
    Fmt::distanceLap(buf, sizeof(buf), Fmt::distUnits(lap.distance, imperial));
    lv_label_set_text(mDistance, buf);
    Fmt::shortTime(buf, sizeof(buf), lap.time);
    lv_label_set_text(mTime, buf);
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(lap.pace, imperial));
    lv_label_set_text(mPace, buf);

    const LapCompare::Delta& d = lap.delta;
    const bool show = d.verdict != LapCompare::Verdict::None;
    Theme::setHidden(mChip, !show);
    Theme::setHidden(mChipText, !show);
    if (show) {
        char line[24];
        LapCompare::line(line, sizeof(line), d);
        lv_label_set_text(mChipText, line);
        const uint32_t fill = d.verdict == LapCompare::Verdict::Faster ? SDK::GUI::Color::LIME_SOFT
                            : d.verdict == LapCompare::Verdict::Slower ? SDK::GUI::Color::SALMON
                                                                        : SDK::GUI::Color::WHITE;
        lv_obj_set_style_bg_color(mChip, Theme::rgb(fill), LV_PART_MAIN);
    }
}

void TrackLapScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackLapScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(self->mModel.isLapFromPauseMenu() ? ScreenId::TrackAction
                                                                      : ScreenId::Track);
}

void TrackLapScreen::onHide()
{
    mModel.setLapFromPauseMenu(false);
}
