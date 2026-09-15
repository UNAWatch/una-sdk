/**
 ******************************************************************************
 * @file    TrackIntervalsCountdownScreen.cpp
 * @brief   Countdown before an intervals workout (see the header).
 ******************************************************************************
 */

#include "gui/screens/TrackIntervalsCountdownScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

using namespace SDK::GUI;

TrackIntervalsCountdownScreen::TrackIntervalsCountdownScreen(Model& model)
    : Screen(model)
{
}

TrackIntervalsCountdownScreen::~TrackIntervalsCountdownScreen()
{
    lv_anim_delete(this, nullptr);
}

void TrackIntervalsCountdownScreen::build()
{
    using F = Theme::Font;
    Theme::hline(mRoot, 63, 113, 114, Color::GRAY_DARK);
    Theme::hline(mRoot, 63, 81, 114, Color::GRAY_DARK);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mRing = std::make_unique<Widgets::TimerRing>(mRoot);
    mRing->setColor(Color::YELLOW_DARK);
    mRing->setRemaining(1000);
    Theme::imageTinted(mRoot, &img_crosswhite_17x17, 187, 163, SDK::GUI::Color::WHITE);
    Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60, SDK::GUI::Color::YELLOW_DARK);
    mCount = Theme::label(mRoot, F::SemiBold60, "5", 70, 142, 100);
    mRest  = Theme::label(mRoot, F::Medium18, "", 40, 119, 160);
    mRun   = Theme::label(mRoot, F::Medium18, "", 40, 87, 160);
    mReps  = Theme::label(mRoot, F::Medium18, "", 40, 55, 160);
}

void TrackIntervalsCountdownScreen::onShow()
{
    mModel.resetIdleTimer();

    const Settings::Intervals& iv = mModel.getSettings().intervals;
    const bool imperial = mModel.isUnitsImperial();
    char buf[24];
    char reps[8];
    Fmt::intervalsRepeats(reps, sizeof(reps), iv.repeatsNum);
    snprintf(buf, sizeof(buf), "Reps: %s", reps);
    lv_label_set_text(mReps, buf);
    Fmt::intervalsPhaseSummary(buf, sizeof(buf), "Run", iv.runMetric, iv.runTime, iv.runDistance, imperial);
    lv_label_set_text(mRun, buf);
    Fmt::intervalsPhaseSummary(buf, sizeof(buf), "Rest", iv.restMetric, iv.restTime, iv.restDistance, imperial);
    lv_label_set_text(mRest, buf);

    mShownSeconds = kTimeoutMs / 1000;
    lv_label_set_text_fmt(mCount, "%u", static_cast<unsigned>(mShownSeconds));

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, static_cast<int32_t>(kTimeoutMs), 0);
    lv_anim_set_duration(&a, kTimeoutMs);
    lv_anim_set_exec_cb(&a, &TrackIntervalsCountdownScreen::animExecCb);
    lv_anim_set_completed_cb(&a, &TrackIntervalsCountdownScreen::animReadyCb);
    lv_anim_start(&a);
}

void TrackIntervalsCountdownScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1) {
        startTrack();
    } else if (code == Btn::R2) {
        lv_anim_delete(this, nullptr);
        ScreenManager::instance().goTo(ScreenId::MenuIntervals);
    }
}

void TrackIntervalsCountdownScreen::animExecCb(void* var, int32_t value)
{
    auto* self = static_cast<TrackIntervalsCountdownScreen*>(var);
    if (value < 0) {
        value = 0;
    }
    self->mRing->setRemaining(static_cast<uint32_t>(value) * 1000u / kTimeoutMs);
    const uint32_t seconds = static_cast<uint32_t>(value) / 1000u;
    if (seconds != self->mShownSeconds) {
        self->mShownSeconds = seconds;
        lv_label_set_text_fmt(self->mCount, "%u", static_cast<unsigned>(seconds));
    }
}

void TrackIntervalsCountdownScreen::animReadyCb(lv_anim_t* a)
{
    static_cast<TrackIntervalsCountdownScreen*>(a->var)->startTrack();
}

void TrackIntervalsCountdownScreen::startTrack()
{
    if (mStarted) {
        return;
    }
    mStarted = true;
    lv_anim_delete(this, nullptr);
    mModel.trackStart(true);
    // With a warm-up the track faces come first; otherwise the RUN alert opens
    // the workout (the model pre-filled its snapshot in trackStart()).
    ScreenManager::instance().goTo(mModel.getSettings().intervals.warmUp ? ScreenId::Track
                                                                          : ScreenId::TrackIntervalsAlert);
}
