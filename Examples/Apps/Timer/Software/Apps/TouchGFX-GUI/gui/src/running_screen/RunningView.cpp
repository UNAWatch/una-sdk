#include <gui/running_screen/RunningView.hpp>
#include <SDK/GUI/Button.hpp>

RunningView::RunningView()
{
}

void RunningView::setupScreen()
{
    RunningViewBase::setupScreen();

    title.set("TIMER");

    touchgfx::Unicode::snprintf(resetLabelBuffer, RESETLABEL_SIZE, "Reset");
    resetLabel.setWildcard(resetLabelBuffer);

    updateTime();
    syncControls();
}

void RunningView::tearDownScreen()
{
    RunningViewBase::tearDownScreen();
}

void RunningView::onStateChanged()
{
    syncControls();
    updateTime();
}

void RunningView::handleTickEvent()
{
    RunningViewBase::handleTickEvent();
    updateTime();
}

void RunningView::updateTime()
{
    // Ceil to seconds so the display reads the full duration at the start and
    // only shows 00:00 at the moment of firing.
    const uint32_t sec = (presenter->getRemainingMs() + 999u) / 1000u;
    touchgfx::Unicode::snprintf(timeTextBuffer, TIMETEXT_SIZE, "%02u:%02u",
                                sec / 60u, sec % 60u);
    timeText.setWildcard(timeTextBuffer);
    timeText.invalidate();
}

void RunningView::syncControls()
{
    const bool running = presenter->getState() == TimerState::RUNNING;

    // Only visibility changes -- every icon is placed in the Designer.
    pauseIcon.setVisible(running);
    playIcon.setVisible(!running);
    stopIcon.setVisible(!running);       // stop is offered once paused

    pauseIcon.invalidate();
    playIcon.invalidate();
    stopIcon.invalidate();

    // Each arc carries the colour of the icon next to it: pause amber, resume
    // teal, stop red, reset white. R2 (back to Main) stays white.
    buttons.setR1(running ? Buttons::AMBER : Buttons::TEAL);
    buttons.setL1(running ? Buttons::NONE  : Buttons::RED);
    buttons.setL2(Buttons::WHITE);
    buttons.setR2(Buttons::WHITE);
}

void RunningView::handleKeyEvent(uint8_t key)
{
    const TimerState state = presenter->getState();

    if (key == SDK::GUI::Button::R1) {
        if (state == TimerState::RUNNING) {
            presenter->pause();
        } else {
            presenter->resume();
        }
    }
    else if (key == SDK::GUI::Button::L1) {
        // Stop ends the countdown and returns to the timer's Menu (Start / Edit
        // / Delete); from there R2 returns to Main.
        if (state == TimerState::PAUSED) {
            presenter->stop();
            application().gotoMenuScreenNoTransition();
        }
    }
    else if (key == SDK::GUI::Button::L2) {
        presenter->reset();
    }
    else if (key == SDK::GUI::Button::R2) {
        // Leave the GUI running in the background (service keeps counting).
        presenter->exit();
    }
}
