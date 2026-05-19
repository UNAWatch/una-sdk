#include <gui/trackdiscardconfirmation_screen/TrackDiscardConfirmationView.hpp>

TrackDiscardConfirmationView::TrackDiscardConfirmationView()
{

}

void TrackDiscardConfirmationView::setupScreen()
{
    TrackDiscardConfirmationViewBase::setupScreen();

    title.set(T_TEXT_APP_NAME_UC);

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);
}

void TrackDiscardConfirmationView::tearDownScreen()
{
    TrackDiscardConfirmationViewBase::tearDownScreen();
}

void TrackDiscardConfirmationView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {

    }

    if (key == SDK::GUI::Button::L2) {

    }

    if (key == SDK::GUI::Button::R1) {
        application().gotoTrackDiscardedScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoTrackActionScreenNoTransition();
    }
}
