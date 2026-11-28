/**
 ******************************************************************************
 * @file    MenuRunAlertsScreen.cpp
 * @brief   Settings > Run Alerts (see MenuRunAlertsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuRunAlertsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuRunAlertsScreen::MenuRunAlertsScreen(Model& model)
    : Screen(model)
{
}

void MenuRunAlertsScreen::build()
{
    mItems[Menu::ID_RUN_WALK]   = { Style::Tip, "Run/Walk",   nullptr, &poppins_semibold_30, mRunWalkTip };
    mItems[Menu::ID_PACE]       = { Style::Tip, "Pace",       nullptr, &poppins_semibold_30, mPaceTip };
    mItems[Menu::ID_HEART_RATE] = { Style::Tip, "Heart Rate", nullptr, &poppins_semibold_30, mHrTip };
    mItems[Menu::ID_BACK_IN]    = { Style::Toggle, "Back in\nRange", "Back in Range", &poppins_semibold_25 };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "RUN ALERTS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuRunAlertsScreen::onShow()
{
    mMenu->select(mModel.menu().settings.runAlerts.get());
    mModel.menu().settings.runAlerts.resetChildren();
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuRunAlertsScreen::onHide()
{
    mModel.menu().settings.runAlerts.set(mMenu->selected());
}

void MenuRunAlertsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            switch (mMenu->selected()) {
                case Menu::ID_RUN_WALK:   ScreenManager::instance().goTo(ScreenId::MenuRunWalk); break;
                case Menu::ID_PACE:       ScreenManager::instance().goTo(ScreenId::MenuRunPace); break;
                case Menu::ID_HEART_RATE: ScreenManager::instance().goTo(ScreenId::MenuRunHr);   break;
                default: {
                    Settings sett = mModel.getSettings();
                    sett.runAlerts.backInTap = !sett.runAlerts.backInTap;
                    mModel.saveSettings(sett);
                    onSettings(sett);
                    break;
                }
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuSettings);
            break;
        default:
            break;
    }
}

void MenuRunAlertsScreen::onSettings(const Settings& settings)
{
    // As Lap Alerts: OFF in teal, a setting in amber.
    const Settings::RunAlerts& ra = settings.runAlerts;
    snprintf(mRunWalkTip, sizeof(mRunWalkTip), "%s", ra.runWalkEn ? "ON" : "OFF");
    mItems[Menu::ID_RUN_WALK].tipColor = ra.runWalkEn ? Color::YELLOW_DARK : Color::TEAL;

    RunAlerts::paceRangeText(ra, mModel.isUnitsImperial(), mPaceTip, sizeof(mPaceTip));
    const bool paceOn = mPaceTip[0] != '\0';
    if (!paceOn) {
        snprintf(mPaceTip, sizeof(mPaceTip), "OFF");
    }
    mItems[Menu::ID_PACE].tipColor = paceOn ? Color::YELLOW_DARK : Color::TEAL;

    RunAlerts::hrRangeText(ra, mHrTip, sizeof(mHrTip));
    const bool hrOn = mHrTip[0] != '\0';
    if (!hrOn) {
        snprintf(mHrTip, sizeof(mHrTip), "OFF");
    }
    mItems[Menu::ID_HEART_RATE].tipColor = hrOn ? Color::YELLOW_DARK : Color::TEAL;

    mItems[Menu::ID_BACK_IN].toggleState = ra.backInTap;
    mMenu->refresh();
}

void MenuRunAlertsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuRunAlertsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
