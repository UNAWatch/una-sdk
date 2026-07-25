#ifndef TIMER_MANAGER_HPP
#define TIMER_MANAGER_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <string_view>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "Timer.hpp"

/**
 * @brief Manages the timer list, persistence, and timed triggering.
 *
 * Responsibilities:
 * - Load / save timers from / to JSON file on the filesystem
 * - Check every minute (via execute()) whether any timer is due
 * - Track snoozed timers and re-trigger them after kSnoozedTimeMinutes
 * - Notify an observer on timer trigger and on list changes
 */
class TimerManager {
public:

    /**
     * @brief Observer interface for timer events.
     */
    class TimerCallback {
    public:
        /** @brief Called when an timer fires. */
        virtual void onTimer(const Timer& timer) {}

        /** @brief Called when the timer list changes (load, save). */
        virtual void onListChanged(const std::vector<Timer>& list) {}
    protected:
        virtual ~TimerCallback() = default;
    };

    TimerManager(const SDK::Kernel& kernel);
    virtual ~TimerManager();

    /** @brief Load timers from persistent storage and notify observer. */
    void load();

    /** @brief Attach observer to receive timer and list-change events. */
    void attachCallback(TimerCallback* pCallback)
    {
        mObserver = pCallback;
    }

    /**
     * @brief Check for due timers; call once per minute.
     * @param tmNow Current local time.
     * @return Milliseconds until the next required call.
     */
    uint32_t execute(const std::tm& tmNow);

    /** @brief Return the current timer list (non-owning reference). */
    const std::vector<Timer>& getTimerList();

    /**
     * @brief Overwrite the timer list and persist to storage.
     * @return true if saved successfully.
     */
    bool saveTimerList(const std::vector<Timer>& list);

    /** @brief Remove a specific timer from the snoozed-timer tracking list. */
    void disableTimer(const Timer& timer);

    /** @brief Clear the entire snoozed-timer tracking list. */
    void disableAllActiveTimer();

    /**
     * @brief Acknowledge snooze for a specific timer.
     *
     * The timer is already tracked in the snoozed list (added automatically
     * when it first fired). This call is a no-op by design; re-triggering
     * is handled by execute() on the next snooze interval.
     */
    void snoozeTimer(const Timer& timer);

    /**
     * @brief Acknowledge snooze for all active timers.
     *
     * Same semantics as snoozeTimer() — no-op by design.
     */
    void snoozeAllActiveTimer();

    /** @brief Return true if any enabled or snoozed timer exists. */
    bool hasActiveTimers() const;

private:

    // -- Constants ------------------------------------------------------------

    static constexpr char    skFilePath[]        = "timers.json";
    static constexpr uint8_t kSnoozedTimeMinutes = 5;
    static constexpr uint8_t kMaxSnoozeCount     = 5;
    static constexpr size_t  kInitialCount       = 20;

    // -- State ----------------------------------------------------------------

    const SDK::Kernel&      mKernel;
    TimerCallback*          mObserver = nullptr;
    std::vector<Timer>      mTimers{};
    char                    mBuffer[2048]{};

    // -- JSON key maps (index = enum value) -----------------------------------

    inline static constexpr std::array<std::string_view, Timer::REPEAT_COUNT> kRepeatJsonKeyValue =
    { "no", "every_day", "week_days", "weekends",
      "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday" };

    inline static constexpr std::array<std::string_view, Timer::EFFECT_COUNT> kEffectJsonKeyValue =
    { "beep_vibro", "vibro", "beep" };

    // -- Snoozed-timer tracking -----------------------------------------------

    struct SnoozedTimer {
        Timer   info;
        uint8_t snoozeCount       = kMaxSnoozeCount;
        uint8_t nextTriggerHour   = 0;
        uint8_t nextTriggerMinute = 0;
    };

    std::vector<SnoozedTimer> mSnoozedTimers;

    // -- Helpers --------------------------------------------------------------

    bool     saveToFile(const std::vector<Timer>& timers);
    bool     loadFromFile(std::vector<Timer>& timers);
    uint32_t createJSON(const std::vector<Timer>& timers, char* buff, uint32_t buffSize);
    bool     parseJSON(char* buff, uint32_t length, std::vector<Timer>& timers);
    void     dump(const std::vector<Timer>& timers);

    void checkTimers(uint8_t currentHour, uint8_t currentMinute,
                     uint8_t currentDay, const std::tm& tmNow);
    void addSnoozedTimer(const Timer& timer, const std::tm& tmNow);
    void updateSnoozedTriggerTime(SnoozedTimer& snoozed, const std::tm& tmNow);
    void removeObsoleteSnoozedTimers();

    bool isTimerDueToday(const Timer& timer, uint8_t currentDay) const;
    bool isSnoozed(const Timer& timer) const;
};

#endif // TIMER_MANAGER_HPP
