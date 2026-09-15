/**
 ******************************************************************************
 * @file    MenuAlertsScreen.hpp
 * @brief   Lap alerts menu: Distance and Time auto-lap settings with their
 *          current values as hints.
 ******************************************************************************
 */

#ifndef MENU_ALERTS_SCREEN_HPP
#define MENU_ALERTS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuAlertsScreen : public Screen
{
public:
    explicit MenuAlertsScreen(Model& model);

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
    using Menu = App::MenuNav::Root::Settings::Alerts;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mDistanceTip[16] = "";
    char mTimeTip[16]     = "";

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_ALERTS_SCREEN_HPP
