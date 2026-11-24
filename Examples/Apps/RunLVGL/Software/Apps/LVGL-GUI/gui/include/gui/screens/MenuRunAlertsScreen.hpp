/**
 ******************************************************************************
 * @file    MenuRunAlertsScreen.hpp
 * @brief   Settings > Run Alerts: Run/Walk, Pace and Heart Rate (each a
 *          submenu, its state as the hint) and the Back in Range toggle.
 ******************************************************************************
 */

#ifndef MENU_RUN_ALERTS_SCREEN_HPP
#define MENU_RUN_ALERTS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuRunAlertsScreen : public Screen
{
public:
    explicit MenuRunAlertsScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }
    void onSettings(const Settings& settings) override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Settings::RunAlerts;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mRunWalkTip[8] = "";
    char mPaceTip[16]   = "";
    char mHrTip[16]     = "";

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_RUN_ALERTS_SCREEN_HPP
