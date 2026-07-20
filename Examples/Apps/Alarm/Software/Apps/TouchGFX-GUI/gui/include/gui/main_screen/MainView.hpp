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

    /** @brief Select 12- or 24-hour presentation of the alarm time. */
    void setTimeFormat(bool is12Hour) { mIs12Hour = is12Hour; }

protected:
    /// Current alarm index. Equals pList->size() when the "New Alarm" slot is selected.
    size_t mAlarmId = 0;

    /// Pointer to the alarm list owned by the Model (valid between activate/deactivate).
    const std::vector<Alarm>* pList = nullptr;

    virtual void handleKeyEvent(uint8_t key) override;

    /** @brief Rebuild all visible widgets from the current mAlarmId. */
    void show();

    /// 12-hour presentation of the alarm time (from the system clock setting).
    bool mIs12Hour = false;

    // AM/PM suffix drawn beside the big time in 12-hour mode. The time and the
    // suffix are centred as one group (a two-digit hour widens the group).
    touchgfx::TextAreaWithOneWildcard mMeridiem;
    static const uint16_t MERIDIEM_SIZE = 3;                 // "AM"/"PM" + NUL
    touchgfx::Unicode::UnicodeChar mMeridiemBuffer[MERIDIEM_SIZE];

    static const int16_t kTimeY       = 82;   // matches the generated timeValue Y
    static const int16_t kTimeH       = 77;
    static const int16_t kMeridiemY   = 122;  // SemiBold-60 baseline 60, -20 for 18/20px
    static const int16_t kMeridiemH   = 30;
    static const int16_t kMeridiemGap = 5;
};

#endif // MAINVIEW_HPP
