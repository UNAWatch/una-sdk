#ifndef ALARM_MANAGER_HPP
#define ALARM_MANAGER_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <string_view>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "Alarm.hpp"

/**
 * @brief Manages the alarm list, persistence, and timed triggering.
 *
 * Responsibilities:
 * - Load / save alarms from / to JSON file on the filesystem
 * - Check every minute (via execute()) whether any alarm is due
 * - Track snoozed alarms and re-trigger them after kSnoozedTimeMinutes
 * - Notify an observer on alarm trigger and on list changes
 */
class AlarmManager {
public:

    /**
     * @brief Observer interface for alarm events.
     */
    class AlarmCallback {
    public:
        /** @brief Called when an alarm fires. */
        virtual void onAlarm(const Alarm& alarm) {}

        /** @brief Called when the alarm list changes (load, save). */
        virtual void onListChanged(const std::vector<Alarm>& list) {}
    protected:
        virtual ~AlarmCallback() = default;
    };

    AlarmManager(const SDK::Kernel& kernel);
    virtual ~AlarmManager();

    /** @brief Load alarms from persistent storage and notify observer. */
    void load();

    /** @brief Attach observer to receive alarm and list-change events. */
    void attachCallback(AlarmCallback* pCallback)
    {
        mObserver = pCallback;
    }

    /**
     * @brief Check for due alarms; call once per minute.
     * @param tmNow Current local time.
     * @return Milliseconds until the next required call.
     */
    uint32_t execute(const std::tm& tmNow);

    /** @brief Return the current alarm list (non-owning reference). */
    const std::vector<Alarm>& getAlarmList();

    /**
     * @brief Overwrite the alarm list and persist to storage.
     * @return true if saved successfully.
     */
    bool saveAlarmList(const std::vector<Alarm>& list);

    /** @brief Remove a specific alarm from the snoozed-alarm tracking list. */
    void disableAlarm(const Alarm& alarm);

    /** @brief Clear the entire snoozed-alarm tracking list. */
    void disableAllActiveAlarm();

    /**
     * @brief Acknowledge snooze for a specific alarm.
     *
     * The alarm is already tracked in the snoozed list (added automatically
     * when it first fired). This call is a no-op by design; re-triggering
     * is handled by execute() on the next snooze interval.
     */
    void snoozeAlarm(const Alarm& alarm);

    /**
     * @brief Acknowledge snooze for all active alarms.
     *
     * Same semantics as snoozeAlarm() — no-op by design.
     */
    void snoozeAllActiveAlarm();

    /** @brief Return true if any enabled or snoozed alarm exists. */
    bool hasActiveAlarms() const;

private:

    // -- Constants ------------------------------------------------------------

    static constexpr char    skFilePath[]        = "alarms.json";
    static constexpr uint8_t kSnoozedTimeMinutes = 5;
    static constexpr uint8_t kMaxSnoozeCount     = 5;
    static constexpr size_t  kInitialCount       = 20;

    // -- State ----------------------------------------------------------------

    const SDK::Kernel&      mKernel;
    AlarmCallback*          mObserver = nullptr;
    std::vector<Alarm>      mAlarms{};
    char                    mBuffer[2048]{};

    // -- JSON key maps (index = enum value) -----------------------------------

    inline static constexpr std::array<std::string_view, Alarm::REPEAT_COUNT> kRepeatJsonKeyValue =
    { "no", "every_day", "week_days", "weekends",
      "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday" };

    inline static constexpr std::array<std::string_view, Alarm::EFFECT_COUNT> kEffectJsonKeyValue =
    { "beep_vibro", "vibro", "beep" };

    // -- Snoozed-alarm tracking -----------------------------------------------

    struct SnoozedAlarm {
        Alarm   info;
        uint8_t snoozeCount       = kMaxSnoozeCount;
        uint8_t nextTriggerHour   = 0;
        uint8_t nextTriggerMinute = 0;
    };

    std::vector<SnoozedAlarm> mSnoozedAlarms;

    // -- Helpers --------------------------------------------------------------

    bool     saveToFile(const std::vector<Alarm>& alarms);
    bool     loadFromFile(std::vector<Alarm>& alarms);
    uint32_t createJSON(const std::vector<Alarm>& alarms, char* buff, uint32_t buffSize);
    bool     parseJSON(char* buff, uint32_t length, std::vector<Alarm>& alarms);
    void     dump(const std::vector<Alarm>& alarms);

    void checkAlarms(uint8_t currentHour, uint8_t currentMinute,
                     uint8_t currentDay, const std::tm& tmNow);
    void addSnoozedAlarm(const Alarm& alarm, const std::tm& tmNow);
    void updateSnoozedTriggerTime(SnoozedAlarm& snoozed, const std::tm& tmNow);
    void removeObsoleteSnoozedAlarms();

    bool isAlarmDueToday(const Alarm& alarm, uint8_t currentDay) const;
    bool isSnoozed(const Alarm& alarm) const;
};

#endif // ALARM_MANAGER_HPP
