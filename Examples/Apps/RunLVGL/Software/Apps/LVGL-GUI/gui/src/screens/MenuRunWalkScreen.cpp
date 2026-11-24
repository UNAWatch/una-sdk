/**
 ******************************************************************************
 * @file    MenuRunWalkScreen.cpp
 * @brief   Settings > Run Alerts > Run/Walk (see MenuRunWalkScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuRunWalkScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuRunWalkScreen::MenuRunWalkScreen(Model& model)
    : Screen(model)
{
}

void MenuRunWalkScreen::build()
{
    // A plain on/off row: the label reads the state, set in onSettings().
    mItems[Menu::ID_STATUS]    = { Style::Toggle, "Off", nullptr, &poppins_semibold_25 };
    mItems[Menu::ID_RUN_TIME]  = { Style::Tip, "Run Time",  nullptr, &poppins_semibold_25, mRunTip,
                                   Color::YELLOW_DARK };
    mItems[Menu::ID_WALK_TIME] = { Style::Tip, "Walk Time", nullptr, &poppins_semibold_25, mWalkTip,
                                   Color::YELLOW_DARK };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "RUN/WALK");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuRunWalkScreen::onShow()
{
    mMenu->select(mModel.menu().settings.runAlerts.runWalk.get());
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuRunWalkScreen::onHide()
{
    mModel.menu().settings.runAlerts.runWalk.set(mMenu->selected());
}

void MenuRunWalkScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mMenu->selected() == Menu::ID_STATUS) {
                Settings sett = mModel.getSettings();
                sett.runAlerts.runWalkEn = !sett.runAlerts.runWalkEn;
                mModel.saveSettings(sett);
                onSettings(sett);
            } else {
                mModel.setRunAlertField(mMenu->selected() == Menu::ID_RUN_TIME
                                            ? RunAlerts::Field::RunTime
                                            : RunAlerts::Field::WalkTime);
                ScreenManager::instance().goTo(ScreenId::MenuRunAlertPicker);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuRunAlerts);
            break;
        default:
            break;
    }
}

void MenuRunWalkScreen::onSettings(const Settings& settings)
{
    const bool imperial = mModel.isUnitsImperial();
    RunAlerts::fieldText(RunAlerts::Field::RunTime, settings.runAlerts, imperial, mRunTip, sizeof(mRunTip));
    RunAlerts::fieldText(RunAlerts::Field::WalkTime, settings.runAlerts, imperial, mWalkTip, sizeof(mWalkTip));
    mItems[Menu::ID_STATUS].toggleState = settings.runAlerts.runWalkEn;
    mItems[Menu::ID_STATUS].text        = settings.runAlerts.runWalkEn ? "On" : "Off";
    mMenu->refresh();
}

void MenuRunWalkScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuRunWalkScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
