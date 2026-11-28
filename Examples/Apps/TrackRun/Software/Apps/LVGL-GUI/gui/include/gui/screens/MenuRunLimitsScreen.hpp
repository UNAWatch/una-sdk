/**
 ******************************************************************************
 * @file    MenuRunLimitsScreen.hpp
 * @brief   Settings > Run Alerts > Pace (Too Fast, Too Slow) or Heart Rate
 *          (High, Low): two limits, each OFF or its value as the hint.
 *
 * One class for both submenus, as MenuIntervalsMetricScreen serves Run and
 * Rest. A pace limit opens Off / Pace; a heart-rate limit opens the bpm list.
 ******************************************************************************
 */

#ifndef MENU_RUN_LIMITS_SCREEN_HPP
#define MENU_RUN_LIMITS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuRunLimitsScreen : public Screen
{
public:
    enum class Kind : uint8_t { Pace, HeartRate };

    MenuRunLimitsScreen(Model& model, Kind kind);

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
    static constexpr uint16_t kRows = 2;  ///< Upper limit first: Too Fast / High

    RunAlerts::Field field(uint16_t row) const;

    Kind            mKind;
    WheelMenu::Item mItems[kRows] {};
    char            mTips[kRows][16] {};

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_RUN_LIMITS_SCREEN_HPP
