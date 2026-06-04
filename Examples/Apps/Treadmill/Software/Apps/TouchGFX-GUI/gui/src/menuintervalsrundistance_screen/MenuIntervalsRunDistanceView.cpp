#include <gui/menuintervalsrundistance_screen/MenuIntervalsRunDistanceView.hpp>
#include <SDK/GUI/Button.hpp>

void MenuIntervalsRunDistanceView::setupScreen()
{
    MenuIntervalsRunDistanceViewBase::setupScreen();
    picker.setTitle(T_TEXT_DISTANCE_UC);
}

void MenuIntervalsRunDistanceView::setDistance(float meters, bool isImperial)
{
    mLogic.seed(meters, isImperial);
    mLogic.render(picker);
}

void MenuIntervalsRunDistanceView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        mLogic.dec();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::L2) {
        mLogic.inc();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::R1) {
        if (!mLogic.atFrac()) {
            mLogic.toFrac();
            mLogic.render(picker);
        } else {
            presenter->saveRunDistance(mLogic.meters());
            application().gotoMenuIntervalsScreenNoTransition();
        }
    } else if (key == SDK::GUI::Button::R2) {
        if (!mLogic.atFrac()) {
            application().gotoMenuIntervalsRunScreenNoTransition();
        } else {
            mLogic.toWhole();
            mLogic.render(picker);
        }
    }
}
