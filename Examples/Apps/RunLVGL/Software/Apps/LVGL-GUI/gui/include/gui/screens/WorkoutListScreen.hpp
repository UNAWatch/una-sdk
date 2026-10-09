/**
 ******************************************************************************
 * @file    WorkoutListScreen.hpp
 * @brief   The workouts on the watch, in the service's order: today's, the
 *          days to come, the days gone by, then the library.
 *
 * Port of the Run app's WorkoutListView/Presenter. While a workout is armed
 * the list starts with a Free run row that clears it. The selected row shows
 * the workout's totals ("!" in amber when a step runs in a simplified form);
 * the others, and the line above the wheel, say when it is planned for. With
 * no workout files the screen says where to copy them.
 ******************************************************************************
 */

#ifndef WORKOUT_LIST_SCREEN_HPP
#define WORKOUT_LIST_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"

class WorkoutListScreen : public Screen
{
public:
    explicit WorkoutListScreen(Model& model);
    ~WorkoutListScreen() override;

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onWorkoutList() override;

protected:
    void build() override;

private:
    static constexpr uint16_t kMaxRows = SDK::Workout::Library::kMaxEntries + 1;  ///< + Free run

    /// The texts of one row: the name and hint selected, and not selected.
    struct RowText {
        char text[64];
        char itemText[64];
        char tip[36];
        char group[24];
    };

    /// Rebuild the rows and the wheel from the model's list, keeping the
    /// selection at @p position where it still exists.
    void setList(uint16_t position);
    /// The workout in row @p row, or -1 for the Free run row.
    int16_t workoutAt(uint16_t row) const;
    /// "Today", the planned date, or "Library", for row @p row.
    void groupOf(uint16_t row, char* buf, size_t cap) const;
    void showGroup(uint16_t row);

    const SDK::Workout::Library* mLibrary = nullptr;
    bool     mFreeRunRow = false;
    uint16_t mRows       = 0;

    // One per row, allocated for the list's rows when it is set.
    std::unique_ptr<WheelMenu::Item[]> mItems;
    std::unique_ptr<RowText[]>         mText;

    lv_obj_t* mMenuBox = nullptr;  ///< Holds the wheel, hidden when there are no workouts
    lv_obj_t* mInfo    = nullptr;
    lv_obj_t* mEmpty[3] = {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // WORKOUT_LIST_SCREEN_HPP
