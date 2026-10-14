/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Pre-activity menu: Start / Workouts / Intervals / Settings on the
 *          wheel, with the sensor-status row and the app title.
 *
 * Port of the Run app's MainView + MainPresenter. Start needs a GPS fix; without
 * one it asks for confirmation first. With a workout armed, Start names it
 * and runs it after the countdown. When the workout list arrives with one
 * scheduled for today, the today prompt is offered once.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MainScreen : public Screen
{
public:
    explicit MainScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onIdleTimeout() override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;
    void onWorkoutList() override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root;

    void confirm();
    void updateBackground();
    /// Offer today's scheduled workout, once, when the list arrives.
    void checkTodayPrompt();

    bool mGpsFix = false;
    bool mArmed  = false;
    char mArmedName[64] = "";
    WheelMenu::Item mItems[Menu::ID_COUNT] {};

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MAIN_SCREEN_HPP
