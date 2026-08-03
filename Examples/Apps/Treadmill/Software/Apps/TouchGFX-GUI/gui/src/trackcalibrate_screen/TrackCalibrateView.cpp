#include <gui/trackcalibrate_screen/TrackCalibrateView.hpp>
#include <SDK/GUI/Button.hpp>

void TrackCalibrateView::setupScreen()
{
    TrackCalibrateViewBase::setupScreen();
    picker.setTitle(T_TEXT_DISTANCE_UC);
}

void TrackCalibrateView::setDistance(float meters, bool isImperial)
{
    mLogic.seed(meters, isImperial);
    mLogic.render(picker);
}

void TrackCalibrateView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        mLogic.dec();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::L2) {
        mLogic.inc();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::R1) {     // advance a place / confirm
        if (mLogic.advance()) {
            mLogic.render(picker);
        } else {
            presenter->applyCalibration(mLogic.meters());
            application().gotoTrackSavedScreenNoTransition();
        }
    } else if (key == SDK::GUI::Button::R2) {     // step back a place / back out
        if (mLogic.retreat()) {
            mLogic.render(picker);
        } else {
            // Back out to the pause menu (Resume / Discard / Save) without
            // saving — the activity is still only paused, not stopped.
            application().gotoTrackActionScreenNoTransition();
        }
    }
}
