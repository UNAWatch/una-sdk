#include <gui/menurunalertpicker_screen/MenuRunAlertPickerView.hpp>
#include <gui/common/RunAlertsUi.hpp>
#include <SDK/GUI/Button.hpp>

void MenuRunAlertPickerView::setupScreen()
{
    MenuRunAlertPickerViewBase::setupScreen();
}

void MenuRunAlertPickerView::setValue(RunAlerts::Field field, bool imperial, uint32_t seconds,
                                      uint32_t lo, uint32_t hi)
{
    mField    = field;
    mImperial = imperial;
    picker.setTitle(RunAlertsUi::title(field));
    mLogic.seed(seconds, lo, hi);
    render();
}

void MenuRunAlertPickerView::render()
{
    // A pace's unit goes in the subtitles ("Min/km"): the title has no room.
    // "Min/km" is wider than "Mins.", so it is inset clear of the L1 arc.
    if (RunAlerts::isPace(mField)) {
        mLogic.render(picker, mImperial ? T_TEXT_PICKER_MIN_MI : T_TEXT_PICKER_MIN_KM, T_TEXT_PICKER_SEC, true);
    } else {
        mLogic.render(picker);
    }
}

void MenuRunAlertPickerView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        mLogic.dec();
        render();
    } else if (key == SDK::GUI::Button::L2) {
        mLogic.inc();
        render();
    } else if (key == SDK::GUI::Button::R1) {
        // First press moves to the seconds, second press saves.
        if (!mLogic.atSec()) {
            mLogic.toSec();
            render();
        } else {
            presenter->save(mLogic.totalSeconds());
            application().gotoMenuRunAlertSavedScreenNoTransition();
        }
    } else if (key == SDK::GUI::Button::R2) {
        // Back to the minutes, or out of the picker from them.
        if (mLogic.atSec()) {
            mLogic.toMin();
            render();
        } else if (RunAlerts::isPace(mField)) {
            application().gotoMenuRunPaceLimitScreenNoTransition();
        } else {
            application().gotoMenuRunWalkScreenNoTransition();
        }
    }
}
