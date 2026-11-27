/**
 ******************************************************************************
 * @file    MenuIntervalsRepeatsScreen.hpp
 * @brief   Repeats picker: Open, x1 .. x20 on the wheel.
 ******************************************************************************
 */

#ifndef MENU_INTERVALS_REPEATS_SCREEN_HPP
#define MENU_INTERVALS_REPEATS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuIntervalsRepeatsScreen : public Screen
{
public:
    explicit MenuIntervalsRepeatsScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Intervals::Repeats;

    WheelMenu::Item mItems[Menu::kMaxCount] {};
    char            mTexts[Menu::kMaxCount][8] {};

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_INTERVALS_REPEATS_SCREEN_HPP
