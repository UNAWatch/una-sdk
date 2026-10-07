#ifndef WORKOUTDETAILSVIEW_HPP
#define WORKOUTDETAILSVIEW_HPP

#include <gui_generated/workoutdetails_screen/WorkoutDetailsViewBase.hpp>
#include <gui/workoutdetails_screen/WorkoutDetailsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>
#include <gui/containers/WorkoutLabel.hpp>

/**
 * A workout in full, to read before choosing it: a first page with its name,
 * totals and description, then its steps on the menu wheel, one per row. A
 * repeat is a row of its own ahead of the steps it repeats. View only: to
 * start the workout, go back and choose Start.
 */
class WorkoutDetailsView : public WorkoutDetailsViewBase
{
public:
    WorkoutDetailsView();
    virtual ~WorkoutDetailsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// The first page, from the list entry.
    void setEntry(const SDK::Workout::Library::Entry& entry, bool imperial);
    /// The steps, once the workout has been read; null if it could not be.
    void setProgram(const SDK::Workout::Program* program, bool imperial);

protected:
    struct Row {
        uint16_t step;    ///< Index in the program
        bool     repeat;  ///< The repeat step heading its block
    };

    void showFirstPage(bool show);
    void showRow(int16_t row);
    /// Label and tip for row @p row: duration or kind, then target.
    void rowText(int16_t row, touchgfx::Unicode::UnicodeChar* label, touchgfx::Unicode::UnicodeChar* tip,
                 uint16_t cap) const;

    const SDK::Workout::Program* mProgram = nullptr;
    bool     mImperial = false;
    bool     mFirstPage = true;
    int16_t  mDescY = 160;  ///< Centre of the description's first line
    Row      mRows[SDK::Workout::kMaxSteps] {};
    uint16_t mRowCount = 0;

    WorkoutLabel<32> mName1;
    WorkoutLabel<32> mName2;
    WorkoutLabel<32> mTotals;
    touchgfx::Box    mDivider;
    WorkoutLabel<48> mDesc1;
    WorkoutLabel<48> mDesc2;
    WorkoutLabel<48> mDesc3;

    char mTitle[16] {};
    touchgfx::Unicode::UnicodeChar mInfo[32] {};
    touchgfx::Unicode::UnicodeChar mLabel[32] {};
    touchgfx::Unicode::UnicodeChar mTip[40] {};

    touchgfx::Callback<WorkoutDetailsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<WorkoutDetailsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<WorkoutDetailsView, int16_t>                      mAnimationMiddleCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onAnimationMiddle(int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // WORKOUTDETAILSVIEW_HPP
