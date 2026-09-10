#ifndef ALARM_MANAGER_HPP
#define ALARM_MANAGER_HPP

#include <cstdint>
#include <ctime>
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
 * - Decide on every execute() whether any alarm is due
 * - Track snoozed alarms and re-trigger them after kSnoozedTimeMinutes
 * - Notify an observer on alarm trigger and on list changes
 *
 * @note execute() is **idempotent within a clock minute**: it may be called far
 *       more often than once a minute (every message the service receives
 *       restarts its loop, and a resident service is handed other events too),
 *       and an alarm rings at most once per minute however often it is called.
 */
class AlarmManager {
public:

    /// execute() returns this when nothing is armed: no enabled alarm and no
    /// pending snooze, so the service has no reason to wake on a timer at all.
    static constexpr uint32_t kNoWork = 0xFFFFFFFFu;

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

    /** @brief Load alarms and pending snoozes from storage and notify observer. */
    void load();

    /** @brief Attach observer to receive alarm and list-change events. */
    void attachCallback(AlarmCallback* pCallback)
    {
        mObserver = pCallback;
    }

    /**
     * @brief Check for due alarms. Safe to call at any rate.
     *
     * @param tmNow  Current **local** time; an alarm's hour/minute is matched
     *               against this, because an alarm is a wall-clock event.
     * @param nowUtc The same instant as an absolute UTC timestamp. Snooze
     *               deadlines are measured on this rather than on local
     *               hour/minute, so a timezone change or a clock step cannot
     *               make a pending snooze miss its slot and stick.
     * @return Milliseconds until the next required call, or kNoWork when
     *         nothing is armed and no timed wake-up is needed.
     */
    uint32_t execute(const std::tm& tmNow, std::time_t nowUtc);

    /** @brief Return the current alarm list (non-owning reference). */
    const std::vector<Alarm>& getAlarmList();

    /**
     * @brief Overwrite the alarm list and persist to storage.
     * @return true if saved successfully.
     */
    bool saveAlarmList(const std::vector<Alarm>& list);

    /**
     * @brief Stop a specific alarm: drop any snooze pending for it.
     *
     * Stopping does not let the same minute ring again — that is what the
     * fired-this-minute latch is for; see mFiredAlarms.
     */
    void disableAlarm(const Alarm& alarm);

    /** @brief Stop every ringing / snoozed alarm. */
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

    /** @brief Return true if any enabled alarm or pending snooze exists. */
    bool hasActiveAlarms() const;

private:

    // -- Constants ------------------------------------------------------------

    static constexpr char    skFilePath[]        = "alarms.json";
    static constexpr char    skSnoozeFilePath[]  = "snoozes.json";
    static constexpr uint8_t kSnoozedTimeMinutes = 5;
    /// Automatic re-rings per snooze. Matches the count the previous
    /// decrement-then-test loop actually delivered.
    static constexpr uint8_t kMaxSnoozeRings     = 4;
    static constexpr size_t  kInitialCount       = 20;
    /// Cap on tracked snoozes, so a persisted file can never grow unbounded.
    static constexpr size_t  kMaxSnoozes         = 8;
    /// A snooze this far past its deadline is dropped rather than rung: the
    /// watch was off, or the clock stepped forward, and ringing now would be
    /// ringing at the wrong time.
    static constexpr std::time_t kSnoozeLateGraceSec = 5 * 60;

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

    /**
     * @brief An alarm waiting to ring again after being snoozed.
     *
     * Persisted (see skSnoozeFilePath): a reboot, an OTA or a shutdown gives a
     * service no notice, so a pending snooze has to be written when it changes
     * or it is silently lost.
     */
    struct SnoozedAlarm {
        Alarm       info;
        /// Automatic re-rings still owed. Never 0 for a live entry.
        uint8_t     ringsLeft     = kMaxSnoozeRings;
        /// Absolute UTC deadline of the next ring.
        std::time_t nextTriggerAt = 0;
        /// The parent alarm's `on` was cleared by its own trigger (a one-time
        /// alarm), not by the user. Such an entry must outlive `!on`, or the
        /// snooze the user just asked for is cancelled by the trigger itself.
        bool        detached      = false;
    };

    std::vector<SnoozedAlarm> mSnoozedAlarms;
    /// Set by anything that changes mSnoozedAlarms; flushed to storage at the
    /// end of the public call that changed it.
    bool                      mSnoozesDirty = false;

    /**
     * @brief An alarm that has already rung in the current clock minute.
     *
     * This is a de-duplication latch, not a snooze. Stopping an alarm drops its
     * snooze, and the snooze is what used to keep checkAlarms() from ringing the
     * same minute over again — so Stop, pressed inside the ringing minute, made
     * the alarm ring straight back and left a fresh snooze behind it. The latch
     * is deliberately not cleared by Stop, and is not persisted.
     */
    struct FiredAlarm {
        Alarm   info;
        int64_t minute = 0;   ///< Epoch minute in which it rang.
    };

    std::vector<FiredAlarm> mFiredAlarms;

    // -- Helpers --------------------------------------------------------------

    bool     saveToFile(const std::vector<Alarm>& alarms);
    bool     loadFromFile(std::vector<Alarm>& alarms);
    uint32_t createJSON(const std::vector<Alarm>& alarms, char* buff, uint32_t buffSize);
    bool     parseJSON(char* buff, uint32_t length, std::vector<Alarm>& alarms);
    void     dump(const std::vector<Alarm>& alarms);

    bool     saveSnoozesToFile();
    bool     loadSnoozesFromFile();
    uint32_t createSnoozeJSON(char* buff, uint32_t buffSize);
    bool     parseSnoozeJSON(char* buff, uint32_t length);
    void     persistSnoozesIfDirty();

    void checkAlarms(uint8_t currentHour, uint8_t currentMinute,
                     uint8_t currentDay, std::time_t nowUtc);
    /**
     * @brief Drop or re-anchor snoozes that can no longer ring on time.
     *
     * Runs before the alarm scan: a pending snooze suppresses its own alarm,
     * so an entry that is past saving has to be gone by then, or it costs the
     * alarm a whole day.
     */
    void settleSnoozes(std::time_t nowUtc);
    std::time_t snoozeDeadline(std::time_t nowUtc) const;
    void armSnooze(const Alarm& alarm, std::time_t nowUtc, bool detached);
    void removeObsoleteSnoozedAlarms();

    void markFired(const Alarm& alarm, int64_t nowMinute);
    bool hasFiredThisMinute(const Alarm& alarm, int64_t nowMinute) const;
    void forgetStaleFired(int64_t nowMinute);

    bool isAlarmDueToday(const Alarm& alarm, uint8_t currentDay) const;
    bool hasPendingSnooze(const Alarm& alarm) const;
};

#endif // ALARM_MANAGER_HPP
