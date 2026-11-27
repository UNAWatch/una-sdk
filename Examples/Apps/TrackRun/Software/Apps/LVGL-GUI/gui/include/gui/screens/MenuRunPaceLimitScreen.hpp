/**
 ******************************************************************************
 * @file    MenuRunPaceLimitScreen.hpp
 * @brief   Too Fast or Too Slow (Model::runAlertField()): Off, or Pace, which
 *          opens the min:sec picker. As Intervals' Run and Rest open
 *          Time / Distance / Open.
 ******************************************************************************
 */

#ifndef MENU_RUN_PACE_LIMIT_SCREEN_HPP
#define MENU_RUN_PACE_LIMIT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuRunPaceLimitScreen : public Screen
{
public:
    explicit MenuRunPaceLimitScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Settings::RunAlerts::PaceLimit;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char            mPaceTip[16] = "";

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_RUN_PACE_LIMIT_SCREEN_HPP
