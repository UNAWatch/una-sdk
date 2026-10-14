#ifndef WORKOUTMENUVIEW_HPP
#define WORKOUTMENUVIEW_HPP

#include <gui_generated/workoutmenu_screen/WorkoutMenuViewBase.hpp>
#include <gui/workoutmenu_screen/WorkoutMenuPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * The menu for one workout from the list: Start arms it for the next run,
 * Details shows it in full.
 */
class WorkoutMenuView : public WorkoutMenuViewBase
{
public:
    WorkoutMenuView();
    virtual ~WorkoutMenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// @p group: its date ("SAT 10 OCT"), TODAY or LIBRARY, as the title.
    void setWorkout(const SDK::Workout::Library::Entry& entry, const char* group, bool imperial);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Workouts::Menu;
    static constexpr int16_t kCount = Menu::ID_DETAILS + 1;  ///< No Skip here

    touchgfx::Unicode::UnicodeChar mName[32] {};
    touchgfx::Unicode::UnicodeChar mTotals[32] {};

    touchgfx::Callback<WorkoutMenuView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<WorkoutMenuView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // WORKOUTMENUVIEW_HPP
