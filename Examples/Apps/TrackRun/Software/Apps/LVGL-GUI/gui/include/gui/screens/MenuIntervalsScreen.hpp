/**
 ******************************************************************************
 * @file    MenuIntervalsScreen.hpp
 * @brief   Intervals menu: Start, Repeats, Run, Rest, Warm Up, Cool Down and
 *          Last Rest on the wheel, with the sensor-status row.
 *
 * Port of the Run app's MenuIntervalsView/Presenter. Repeats, Run and Rest show
 * their current setting as a hint; the last three are toggles saved on R1.
 ******************************************************************************
 */

#ifndef MENU_INTERVALS_SCREEN_HPP
#define MENU_INTERVALS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuIntervalsScreen : public Screen
{
public:
    explicit MenuIntervalsScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onIdleTimeout() override;
    void onSettings(const Settings& settings) override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Intervals;

    void applySettings(const Settings& settings);
    void confirm();
    void startIntervals();
    void saveToggle(uint16_t index, bool state);

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mRepeatsTip[16] = "";
    char mRunTip[16]     = "";
    char mRestTip[16]    = "";

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_INTERVALS_SCREEN_HPP
