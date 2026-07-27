#include <gui/fired_screen/FiredView.hpp>
#include <SDK/GUI/Button.hpp>

FiredView::FiredView()
{
}

void FiredView::setupScreen()
{
    FiredViewBase::setupScreen();

    title.set("TIMER");

    const uint16_t sec = presenter->getDurationSec();
    touchgfx::Unicode::snprintf(timeTextBuffer, TIMETEXT_SIZE, "%02u:%02u",
                                sec / 60u, sec % 60u);
    timeText.setWildcard(timeTextBuffer);

    touchgfx::Unicode::snprintf(doneTextBuffer, DONETEXT_SIZE, "Done");
    doneText.setWildcard(doneTextBuffer);

    touchgfx::Unicode::snprintf(repeatTextBuffer, REPEATTEXT_SIZE, "Repeat");
    repeatText.setWildcard(repeatTextBuffer);

    // Done (R1) amber, Repeat (R2) white; left buttons unused.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);
}

void FiredView::tearDownScreen()
{
    FiredViewBase::tearDownScreen();
}

void FiredView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R1) {
        // Done silences and ends the timer. Return to Main when the user was
        // already in the app; leave the GUI when it only opened for the alert.
        presenter->done();
        if (presenter->firedFromBackground()) {
            presenter->exit();
        } else {
            application().gotoMainScreenNoTransition();
        }
    }
    else if (key == SDK::GUI::Button::R2) {
        // Repeat re-arms the countdown paused and shows it on Running.
        presenter->repeat();
        application().gotoRunningScreenNoTransition();
    }
}
