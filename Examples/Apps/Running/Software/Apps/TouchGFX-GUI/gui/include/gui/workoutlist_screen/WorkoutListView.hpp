#ifndef WORKOUTLISTVIEW_HPP
#define WORKOUTLISTVIEW_HPP

#include <gui_generated/workoutlist_screen/WorkoutListViewBase.hpp>
#include <gui/workoutlist_screen/WorkoutListPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>
#include <gui/containers/WorkoutLabel.hpp>

/**
 * The workouts on the watch, in the service's order: today's, the days to
 * come, the days gone by, then the library. While a workout is armed the
 * list starts with a Free run row that clears it.
 */
class WorkoutListView : public WorkoutListViewBase
{
public:
    WorkoutListView();
    virtual ~WorkoutListView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// @p library may be null before the list arrives.
    /// @p today: today's date as YYYYMMDD, to label today's workout.
    void setList(const SDK::Workout::Library* library, bool freeRunRow, bool imperial, uint32_t today);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    /// The workout in row @p row, or -1 for the Free run row.
    int16_t workoutAt(int16_t row) const;
    /// "Today", the planned date, or "Library", for row @p row.
    void    groupOf(int16_t row, touchgfx::Unicode::UnicodeChar* buf, uint16_t cap) const;
    void    showGroup(int16_t row);

    const SDK::Workout::Library* mLibrary = nullptr;
    bool mFreeRunRow = false;
    bool mImperial   = false;
    uint32_t mToday  = 0;

    touchgfx::Unicode::UnicodeChar mText[64] {};
    touchgfx::Unicode::UnicodeChar mTip[32] {};
    touchgfx::Unicode::UnicodeChar mGroup[24] {};

    WorkoutLabel<24> mEmpty1;
    WorkoutLabel<24> mEmpty2;
    WorkoutLabel<24> mEmpty3;

    touchgfx::Callback<WorkoutListView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<WorkoutListView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<WorkoutListView, int16_t>                      mAnimationMiddleCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onAnimationMiddle(int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // WORKOUTLISTVIEW_HPP
