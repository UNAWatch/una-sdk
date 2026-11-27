/**
 ******************************************************************************
 * @file    WorkoutDetailsScreen.hpp
 * @brief   A workout in full, to read before choosing it.
 *
 * Port of the Run app's WorkoutDetailsView/Presenter: a first page with its
 * name, totals and description, then its steps on the menu wheel, one per
 * row. A repeat is a row of its own ahead of the steps it repeats. L2 goes
 * from the first page to the steps and L1 back. View only: R2 returns to the
 * menu the screen came from, where the workout is started.
 ******************************************************************************
 */

#ifndef WORKOUT_DETAILS_SCREEN_HPP
#define WORKOUT_DETAILS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/WorkoutLabel.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

class WorkoutDetailsScreen : public Screen
{
public:
    explicit WorkoutDetailsScreen(Model& model);
    ~WorkoutDetailsScreen() override;

    // Screen
    void onShow() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onWorkoutDetails() override;

protected:
    void build() override;

private:
    struct Row {
        uint16_t step;    ///< Index in the program
        bool     repeat;  ///< The repeat step heading its block
    };

    /// The texts of one row: the label and hint selected, and not selected.
    struct RowText {
        char label[32];
        char itemLabel[32];
        char tip[40];
        char itemTip[40];
    };

    /// The first page, from the list entry.
    void setEntry(const SDK::Workout::Library::Entry& entry);
    /// The steps, once the workout has been read; null if it could not be.
    void setProgram(const SDK::Workout::Program* program);
    void showFirstPage(bool show);
    void showRow(uint16_t row);
    /// Label and hint for row @p row: duration or kind, then target.
    void rowText(uint16_t row, char* label, char* tip, size_t cap) const;
    void back();

    const SDK::Workout::Program* mProgram = nullptr;
    bool     mImperial  = false;
    bool     mFirstPage = true;
    int32_t  mDescY     = 160;  ///< Centre of the description's first line
    uint16_t mRowCount  = 0;

    // One per row, allocated for the workout's rows when it is set.
    std::unique_ptr<Row[]>             mRows;
    std::unique_ptr<WheelMenu::Item[]> mItems;
    std::unique_ptr<RowText[]>         mText;

    WorkoutLabel mName1;
    WorkoutLabel mName2;
    WorkoutLabel mTotals;
    WorkoutLabel mDesc[3];
    lv_obj_t*    mDivider = nullptr;
    lv_obj_t*    mMenuBox = nullptr;  ///< Holds the wheel, hidden on the first page
    lv_obj_t*    mInfo    = nullptr;
    char         mTitleText[16] = "";
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // WORKOUT_DETAILS_SCREEN_HPP
