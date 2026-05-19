#include <gui/trackstartconfirmation_screen/TrackStartConfirmationView.hpp>

TrackStartConfirmationView::TrackStartConfirmationView()
{

}

void TrackStartConfirmationView::setupScreen()
{
    TrackStartConfirmationViewBase::setupScreen();

    title.set(T_TEXT_APP_NAME_UC);

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);
}

void TrackStartConfirmationView::tearDownScreen()
{
    TrackStartConfirmationViewBase::tearDownScreen();
}

void TrackStartConfirmationView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R1) {
        presenter->startTrack();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}
