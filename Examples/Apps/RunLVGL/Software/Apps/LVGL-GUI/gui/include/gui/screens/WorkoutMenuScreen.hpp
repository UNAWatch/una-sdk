/**
 ******************************************************************************
 * @file    WorkoutMenuScreen.hpp
 * @brief   The menu for one workout: Start and Details, plus Skip when it is
 *          today's workout offered at launch.
 *
 * Port of the Run app's WorkoutTodayView and WorkoutMenuView. The workout's
 * name is the line above the wheel and its totals the hint under Start. The
 * title is TODAY for the launch prompt; for a workout chosen from the list it
 * says when the workout is planned for, or WORKOUTS for one from the library.
 * Start arms the workout and returns to the main menu, whose Start then runs
 * it.
 ******************************************************************************
 */

#ifndef WORKOUT_MENU_SCREEN_HPP
#define WORKOUT_MENU_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class WorkoutMenuScreen : public Screen
{
public:
    /// Today: the launch prompt for today's workout. Chosen: the workout
    /// picked in the list.
    enum class Kind { Today, Chosen };

    WorkoutMenuScreen(Model& model, Kind kind);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Workouts::Menu;

    uint16_t workout() const;
    void     confirm();

    Kind     mKind;
    uint16_t mCount;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mName[64]   = "";
    char mTotals[32] = "";

    lv_obj_t* mInfo = nullptr;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // WORKOUT_MENU_SCREEN_HPP
