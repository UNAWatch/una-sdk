/**
 ******************************************************************************
 * @file    MenuIntervalsMetricScreen.hpp
 * @brief   Run / Rest phase menu: Time, Distance or Open.
 *
 * One class for the Run app's MenuIntervalsRunView and MenuIntervalsRestView,
 * which differ only in the settings field they edit.
 ******************************************************************************
 */

#ifndef MENU_INTERVALS_METRIC_SCREEN_HPP
#define MENU_INTERVALS_METRIC_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuIntervalsMetricScreen : public Screen
{
public:
    enum class Phase : uint8_t { Run, Rest };

    MenuIntervalsMetricScreen(Model& model, Phase phase);

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
    using Menu = App::MenuNav::Root::Intervals::Metric;

    void applySettings(const Settings::Intervals& iv);
    void confirm();

    Phase mPhase;
    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mTimeTip[16] = "";
    char mDistTip[16] = "";

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_INTERVALS_METRIC_SCREEN_HPP
