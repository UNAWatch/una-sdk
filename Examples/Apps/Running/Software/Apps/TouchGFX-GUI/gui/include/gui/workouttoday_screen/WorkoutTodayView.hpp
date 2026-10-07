#ifndef WORKOUTTODAYVIEW_HPP
#define WORKOUTTODAYVIEW_HPP

#include <gui_generated/workouttoday_screen/WorkoutTodayViewBase.hpp>
#include <gui/workouttoday_screen/WorkoutTodayPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Offered at launch when today has a scheduled workout that is not yet
 * completed: Start arms it, Details shows it, Skip goes on to the main menu.
 */
class WorkoutTodayView : public WorkoutTodayViewBase
{
public:
    WorkoutTodayView();
    virtual ~WorkoutTodayView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setWorkout(const SDK::Workout::Library::Entry& entry, bool imperial);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Workouts::Menu;

    touchgfx::Unicode::UnicodeChar mName[32] {};
    touchgfx::Unicode::UnicodeChar mTotals[32] {};

    touchgfx::Callback<WorkoutTodayView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<WorkoutTodayView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // WORKOUTTODAYVIEW_HPP
