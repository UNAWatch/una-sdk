#include <gui/calibrationcleared_screen/CalibrationClearedView.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(2, App::Config::kFrameRate);

CalibrationClearedView::CalibrationClearedView()
    : mDismissCb(this, &CalibrationClearedView::onDismiss)
{
}

void CalibrationClearedView::setupScreen()
{
    CalibrationClearedViewBase::setupScreen();

    title.set(T_TEXT_CALIBRATION_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void CalibrationClearedView::tearDownScreen()
{
    mDismissTimer.stop();
    CalibrationClearedViewBase::tearDownScreen();
}

void CalibrationClearedView::onDismiss()
{
    application().gotoMenuCalibrationScreenNoTransition();
}
