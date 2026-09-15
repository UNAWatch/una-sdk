/**
 ******************************************************************************
 * @file    MenuSettingsScreen.hpp
 * @brief   Settings menu: Lap Alerts and the Phone Notifications toggle.
 ******************************************************************************
 */

#ifndef MENU_SETTINGS_SCREEN_HPP
#define MENU_SETTINGS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuSettingsScreen : public Screen
{
public:
    explicit MenuSettingsScreen(Model& model);

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
    using Menu = App::MenuNav::Root::Settings;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_SETTINGS_SCREEN_HPP
