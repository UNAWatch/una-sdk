#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @brief Main screen showing the alarm list with prev/next navigation.
 *
 * Displays one alarm at a time. The user can scroll through all saved alarms
 * plus one extra "New Alarm" slot (when mAlarmId == pList->size()).
 * Pressing R1 opens the action menu for an existing alarm, or the edit screen
 * for the new-alarm slot.
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Replace the displayed list and reset selection to the first item. */
    void updateAlarmList(const std::vector<Alarm>& list);

    /** @brief Highlight the alarm at @p id without replacing the list. */
    void setSelectedAlarm(size_t id);

protected:
    /// Current alarm index. Equals pList->size() when the "New Alarm" slot is selected.
    size_t mAlarmId = 0;

    /// Pointer to the alarm list owned by the Model (valid between activate/deactivate).
    const std::vector<Alarm>* pList = nullptr;

    virtual void handleKeyEvent(uint8_t key) override;

    /** @brief Rebuild all visible widgets from the current mAlarmId. */
    void show();
};

#endif // MAINVIEW_HPP
