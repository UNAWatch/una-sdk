/**
 ******************************************************************************
 * @file    TrackHoldConfirmScreen.cpp
 * @brief   Hold-to-confirm for ending an activity (see the header).
 ******************************************************************************
 */

#include "gui/screens/TrackHoldConfirmScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

using namespace SDK::GUI;

TrackHoldConfirmScreen::TrackHoldConfirmScreen(Model& model)
    : Screen(model)
{
}

TrackHoldConfirmScreen::~TrackHoldConfirmScreen()
{
    lv_anim_delete(this, nullptr);
}

void TrackHoldConfirmScreen::build()
{
    mMode = mModel.getHoldConfirmMode();
    const bool finish = mMode == Model::HoldConfirmMode::Finish;

    mRing = std::make_unique<Widgets::TimerRing>(mRoot);
    mRing->setColor(finish ? Color::CHARTREUSE : Color::RED);

    // R1 confirms (green finish / red discard). No back hint: releasing R1 cancels.
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  finish ? Widgets::Buttons::GREEN : Widgets::Buttons::RED, Widgets::Buttons::NONE);

    Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60,
                       finish ? SDK::GUI::Color::CHARTREUSE : SDK::GUI::Color::RED);
    Theme::label(mRoot, Theme::Font::Medium18, finish ? "Hold to\nFinish" : "Hold to\nDiscard", 53, 67, 133);
    mNumber = Theme::label(mRoot, Theme::Font::SemiBold60, "3", 102, 103, 36);
}

void TrackHoldConfirmScreen::onShow()
{
    mModel.resetIdleTimer();

    // The hold began on the action menu (R1 press); count down right away.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, static_cast<int32_t>(kHoldMs));
    lv_anim_set_duration(&a, kHoldMs);
    lv_anim_set_exec_cb(&a, &TrackHoldConfirmScreen::animExecCb);
    lv_anim_set_completed_cb(&a, &TrackHoldConfirmScreen::animReadyCb);
    lv_anim_start(&a);
}

void TrackHoldConfirmScreen::onKey(uint8_t code)
{
    // Releasing R1 before the countdown completes cancels back to the menu.
    if (code == SDK::GUI::Button::R1_RELEASE) {
        cancel();
    }
}

void TrackHoldConfirmScreen::onIdleTimeout()
{
    // As the TouchGFX presenter: back to the action menu, not through the hold.
    cancel();
}

void TrackHoldConfirmScreen::onSuspend()
{
    // The release cannot reach this screen while the GUI is suspended (the
    // pump drops button codes until resume) and the countdown runs on the
    // wall clock, so left alone it would complete by itself on resume. A
    // suspend mid-hold therefore cancels, like a release would have.
    cancel();
}

void TrackHoldConfirmScreen::cancel()
{
    if (mFired) {
        return;
    }
    lv_anim_delete(this, nullptr);
    ScreenManager::instance().goTo(ScreenId::TrackAction);
}

void TrackHoldConfirmScreen::animExecCb(void* var, int32_t value)
{
    auto* self = static_cast<TrackHoldConfirmScreen*>(var);
    if (value < 0) {
        value = 0;
    }
    self->mRing->setProgress(static_cast<uint32_t>(value) * 1000u / kHoldMs);

    // 0..kHoldMs maps onto 3 -> 2 -> 1; completion fires before 0 would show.
    uint32_t number = 3 - (static_cast<uint32_t>(value) / (kHoldMs / 3));
    if (number < 1) {
        number = 1;
    }
    self->setCountdown(number);
}

void TrackHoldConfirmScreen::animReadyCb(lv_anim_t* a)
{
    auto* self = static_cast<TrackHoldConfirmScreen*>(a->var);
    if (self->mFired) {
        return;
    }
    self->mFired = true;
    ScreenManager::instance().goTo(self->mMode == Model::HoldConfirmMode::Finish
                                       ? ScreenId::TrackSaved
                                       : ScreenId::TrackDiscarded);
}

void TrackHoldConfirmScreen::setCountdown(uint32_t number)
{
    if (number != mShown) {
        mShown = number;
        lv_label_set_text_fmt(mNumber, "%u", static_cast<unsigned>(number));
    }
}
