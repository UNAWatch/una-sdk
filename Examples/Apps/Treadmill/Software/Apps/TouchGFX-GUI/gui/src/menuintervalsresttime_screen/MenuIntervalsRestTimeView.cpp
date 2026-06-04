#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimeView.hpp>
#include <SDK/GUI/Button.hpp>

void MenuIntervalsRestTimeView::setupScreen()
{
    MenuIntervalsRestTimeViewBase::setupScreen();
    picker.setTitle(T_TEXT_TIME_UC);
}

void MenuIntervalsRestTimeView::setTime(uint32_t totalSeconds)
{
    mLogic.seed(totalSeconds);
    mLogic.render(picker);
}

void MenuIntervalsRestTimeView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        mLogic.dec();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::L2) {
        mLogic.inc();
        mLogic.render(picker);
    } else if (key == SDK::GUI::Button::R1) {
        if (!mLogic.atSec()) {
            mLogic.toSec();
            mLogic.render(picker);
        } else {
            presenter->saveRestTime(mLogic.totalSeconds());
            application().gotoMenuIntervalsScreenNoTransition();
        }
    } else if (key == SDK::GUI::Button::R2) {
        if (!mLogic.atSec()) {
            application().gotoMenuIntervalsRestScreenNoTransition();
        } else {
            mLogic.toMin();
            mLogic.render(picker);
        }
    }
}
