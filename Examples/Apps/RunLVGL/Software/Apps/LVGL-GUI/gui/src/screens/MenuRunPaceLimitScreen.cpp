/**
 ******************************************************************************
 * @file    MenuRunPaceLimitScreen.cpp
 * @brief   Too Fast / Too Slow: Off or Pace (see MenuRunPaceLimitScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuRunPaceLimitScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuRunPaceLimitScreen::MenuRunPaceLimitScreen(Model& model)
    : Screen(model)
{
}

void MenuRunPaceLimitScreen::build()
{
    mItems[Menu::ID_OFF]  = { Style::Simple, "Off" };
    mItems[Menu::ID_PACE] = { Style::Tip, "Pace", nullptr, &poppins_semibold_30, mPaceTip,
                              Color::YELLOW_DARK };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, RunAlerts::fieldTitle(mModel.runAlertField()));
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuRunPaceLimitScreen::onShow()
{
    const RunAlerts::Field   f   = mModel.runAlertField();
    const Settings::RunAlerts& ra = mModel.getSettings().runAlerts;
    const bool imperial = mModel.isUnitsImperial();
    const bool on = RunAlerts::fieldValue(f, ra, imperial) > 0;

    // The Pace row shows the limit while it is set; nothing when Off.
    if (on) {
        RunAlerts::fieldText(f, ra, imperial, mPaceTip, sizeof(mPaceTip));
    } else {
        mPaceTip[0] = '\0';
    }
    mMenu->refresh();
    mMenu->select(on ? Menu::ID_PACE : Menu::ID_OFF);
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuRunPaceLimitScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mMenu->selected() == Menu::ID_OFF) {
                mModel.saveRunAlert(0);
                ScreenManager::instance().goTo(ScreenId::MenuRunAlertSaved);
            } else if (!RunAlerts::pickerBounds(mModel.runAlertField(), mModel.getSettings().runAlerts,
                                                mModel.isUnitsImperial()).empty()) {
                // Nothing to pick when the other limit leaves no room (Too
                // Slow at 2:00, say): the row then does nothing.
                ScreenManager::instance().goTo(ScreenId::MenuRunAlertPicker);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuRunPace);
            break;
        default:
            break;
    }
}

void MenuRunPaceLimitScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuRunPaceLimitScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
