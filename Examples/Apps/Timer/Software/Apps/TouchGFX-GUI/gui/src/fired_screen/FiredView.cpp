#include <gui/fired_screen/FiredView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Button.hpp>

// The alert re-plays every kPlayTicks and gives up (auto-Done) after
// kTimeoutTicks, so an unacknowledged timer stops itself after a minute.
static constexpr uint16_t kPlayTicks    = SDK::Utils::secToTicks(5,  App::Config::kFrameRate);
static constexpr uint16_t kTimeoutTicks = SDK::Utils::secToTicks(60, App::Config::kFrameRate);

FiredView::FiredView()
    : mPlayCb(this, &FiredView::onPlay)
    , mTimeoutCb(this, &FiredView::onTimeout)
{
}

void FiredView::setupScreen()
{
    FiredViewBase::setupScreen();

    title.set(T_TEXT_TIMER_UC);

    const uint16_t sec = presenter->getDurationSec();
    touchgfx::Unicode::snprintf(timeTextBuffer, TIMETEXT_SIZE, "%02u:%02u",
                                sec / 60u, sec % 60u);
    timeText.invalidate();

    // Done (R1) amber, Repeat (R2) white; left buttons unused.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    // The service played the first alert on fire; keep it going from here, and
    // arm the one-minute auto-Done. (mPlayTimer restarts itself in onPlay.)
    mPlayTimer.setDuration(kPlayTicks);
    mPlayTimer.setCallback(mPlayCb);
    mPlayTimer.start();

    mTimeoutTimer.setDuration(kTimeoutTicks);
    mTimeoutTimer.setCallback(mTimeoutCb);
    mTimeoutTimer.start();
}

void FiredView::tearDownScreen()
{
    mPlayTimer.stop();
    mTimeoutTimer.stop();
    FiredViewBase::tearDownScreen();
}

void FiredView::onPlay()
{
    presenter->replayAlert();
    mPlayTimer.start();   // schedule the next repeat
}

void FiredView::onTimeout()
{
    finishDone();         // unattended for a minute -> stop as if Done was pressed
}

void FiredView::finishDone()
{
    // Done silences and ends the timer. Return to Main when the user was already
    // in the app; leave the GUI when it only opened for the alert.
    presenter->done();
    if (presenter->firedFromBackground()) {
        presenter->exit();
    } else {
        application().gotoMainScreenNoTransition();
    }
}

void FiredView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R1) {
        finishDone();
    }
    else if (key == SDK::GUI::Button::R2) {
        // Repeat re-arms the countdown paused and shows it on Running.
        presenter->repeat();
        application().gotoRunningScreenNoTransition();
    }
}
