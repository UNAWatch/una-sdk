/**
 ******************************************************************************
 * @file    RunAlertPickerScreen.cpp
 * @brief   The Run Alerts min:sec picker (see RunAlertPickerScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/RunAlertPickerScreen.hpp"
#include "gui/screens/ScreenManager.hpp"

RunAlertPickerScreen::RunAlertPickerScreen(Model& model)
    : Screen(model)
{
}

void RunAlertPickerScreen::build()
{
    mPicker = std::make_unique<Widgets::TwoTonePicker>(mRoot);
    mPicker->setTitle(RunAlerts::fieldTitle(mModel.runAlertField()));
}

void RunAlertPickerScreen::render()
{
    // A pace's unit goes in the subtitles ("Min/km"): the title has no room.
    // "Min/km" is wider than "Mins.", so it is inset clear of the L1 arc.
    if (RunAlerts::isPace(mModel.runAlertField())) {
        mTime.render(*mPicker, mModel.isUnitsImperial() ? "Min/mi" : "Min/km", "Sec", true);
    } else {
        mTime.render(*mPicker);
    }
}

void RunAlertPickerScreen::onShow()
{
    const RunAlerts::Field     f        = mModel.runAlertField();
    const Settings::RunAlerts& ra       = mModel.getSettings().runAlerts;
    const bool                 imperial = mModel.isUnitsImperial();
    const RunAlerts::Bounds    b        = RunAlerts::pickerBounds(f, ra, imperial);
    mTime.seed(RunAlerts::pickerSeed(f, ra, imperial, mModel.getHrThresholds(), mModel.getHrThresholdsCount()),
               b.min, b.max);
    render();
    mModel.resetIdleTimer();
}

void RunAlertPickerScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            mTime.dec();
            render();
            break;
        case Btn::L2:
            mTime.inc();
            render();
            break;
        case Btn::R1:
            // First press moves to the seconds, second press saves.
            if (!mTime.atSec()) {
                mTime.toSec();
                render();
            } else {
                mModel.saveRunAlert(mTime.totalSeconds());
                ScreenManager::instance().goTo(ScreenId::MenuRunAlertSaved);
            }
            break;
        case Btn::R2:
            // Back to the minutes, or out of the picker from them.
            if (mTime.atSec()) {
                mTime.toMin();
                render();
            } else {
                ScreenManager::instance().goTo(RunAlerts::isPace(mModel.runAlertField())
                                                   ? ScreenId::MenuRunPaceLimit
                                                   : ScreenId::MenuRunWalk);
            }
            break;
        default:
            break;
    }
}
