#include <gui/trackcalibrateintro_screen/TrackCalibrateIntroView.hpp>
#include <SDK/GUI/Button.hpp>

void TrackCalibrateIntroView::setupScreen()
{
    TrackCalibrateIntroViewBase::setupScreen();

    title.set(T_TEXT_CALIBRATE_UC);

    // Only OK is offered on this informational screen.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::NONE);
}

void TrackCalibrateIntroView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R1) {
        application().gotoTrackCalibrateScreenNoTransition();
    }
}
