/**
 ******************************************************************************
 * @file    MenuAlertsScreen.cpp
 * @brief   Lap alerts menu (see MenuAlertsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuAlertsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuAlertsScreen::MenuAlertsScreen(Model& model)
    : Screen(model)
{
}

void MenuAlertsScreen::build()
{
    mItems[Menu::ID_DISTANCE] = { Style::Tip, "Distance", nullptr, &poppins_semibold_30, mDistanceTip };
    mItems[Menu::ID_TIME]     = { Style::Tip, "Time",     nullptr, &poppins_semibold_30, mTimeTip };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "LAP ALERTS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuAlertsScreen::onShow()
{
    mMenu->select(mModel.menu().settings.alerts.get());
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuAlertsScreen::onHide()
{
    mModel.menu().settings.alerts.set(mMenu->selected());
}

void MenuAlertsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            ScreenManager::instance().goTo(mMenu->selected() == Menu::ID_DISTANCE
                                               ? ScreenId::MenuAlertDistance
                                               : ScreenId::MenuAlertTime);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuSettings);
            break;
        default:
            break;
    }
}

void MenuAlertsScreen::onSettings(const Settings& settings)
{
    // An active alert reads amber, an inactive one teal (MenuAlertsView::formatTips).
    Fmt::alertDistance(mDistanceTip, sizeof(mDistanceTip), settings.alertDistanceId, mModel.isUnitsImperial());
    mItems[Menu::ID_DISTANCE].tipColor =
        settings.alertDistanceId == Settings::Alerts::Distance::ID_OFF ? Color::TEAL : Color::YELLOW_DARK;
    Fmt::alertTime(mTimeTip, sizeof(mTimeTip), settings.alertTimeId, false);
    mItems[Menu::ID_TIME].tipColor =
        settings.alertTimeId == Settings::Alerts::Time::ID_OFF ? Color::TEAL : Color::YELLOW_DARK;
    mMenu->refresh();
}

void MenuAlertsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuAlertsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
