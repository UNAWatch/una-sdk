#include <gui/calibrationclearconfirm_screen/CalibrationClearConfirmView.hpp>
#include <SDK/GUI/Button.hpp>

void CalibrationClearConfirmView::setupScreen()
{
    CalibrationClearConfirmViewBase::setupScreen();

    title.set(T_TEXT_CALIBRATION_UC);

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);   // confirm
    buttons.setR2(Buttons::WHITE);   // cancel
}

void CalibrationClearConfirmView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R1) {
        presenter->clearCalibration();
        application().gotoCalibrationClearedScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuCalibrationScreenNoTransition();
    }
}
