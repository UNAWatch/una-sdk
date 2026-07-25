#ifndef TIMER_MANAGER_HPP
#define TIMER_MANAGER_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <string_view>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "Timer.hpp"

/**
 * @brief Owns the single active countdown and the recent-timers store.
 *
 * Responsibilities:
 * - Drive one countdown through IDLE -> RUNNING <-> PAUSED -> FIRED using the
 *   kernel's monotonic millisecond tick (start / pause / resume / reset / stop
 *   / repeat). execute() advances it and fires the observer at expiry.
 * - Keep and persist the last kMaxRecents manually entered timers (JSON file).
 *
 * The manager is time-source agnostic beyond the millisecond tick passed into
 * every control call, which makes it host-testable.
 */
class TimerManager {
public:

    /** @brief Observer interface for countdown and recents events. */
    class Callback {
    public:
        /** @brief Called once when the running countdown reaches zero. */
        virtual void onFired(const Timer& timer) {}

        /** @brief Called when the recents list changes (load, save). */
        virtual void onRecentsChanged(const std::vector<Timer>& list) {}
    protected:
        virtual ~Callback() = default;
    };

    /** @brief Immutable snapshot of the countdown for the GUI. */
    struct State {
        TimerState    state       = TimerState::IDLE;
        uint32_t      endTick     = 0;   ///< Absolute expiry tick (RUNNING).
        uint32_t      remainingMs = 0;   ///< Frozen remainder (PAUSED / FIRED).
        uint16_t      durationSec = 0;
        Timer::Effect effect      = Timer::EFFECT_BEEP_AND_VIBRO;
    };

    explicit TimerManager(const SDK::Kernel& kernel);
    virtual ~TimerManager();

    /** @brief Attach observer to receive fire and recents-change events. */
    void attachCallback(Callback* pCallback) { mObserver = pCallback; }

    /** @brief Load recents from persistent storage and notify the observer. */
    void load();

    // -- Countdown control (nowMs = kernel.sys.getTimeMs()) -------------------

    /** @brief Arm and start a fresh countdown. */
    void start(uint16_t durationSec, Timer::Effect effect, uint32_t nowMs);

    /** @brief Freeze a running countdown, keeping the remaining time. */
    void pause(uint32_t nowMs);

    /** @brief Resume a paused countdown from its remaining time. */
    void resume(uint32_t nowMs);

    /** @brief Reload the full duration and hold it paused. */
    void reset(uint32_t nowMs);

    /** @brief Cancel the countdown and return to IDLE. */
    void stop();

    /** @brief Restart the current timer from its full duration (from FIRED). */
    void repeat(uint32_t nowMs);

    /**
     * @brief Advance the countdown and fire the observer on expiry.
     * @param nowMs Current monotonic tick in milliseconds.
     * @return Milliseconds until the next required call, or kNoTimeout when
     *         nothing is scheduled (IDLE / PAUSED / FIRED).
     */
    uint32_t execute(uint32_t nowMs);

    /** @brief Return true while a countdown is RUNNING or PAUSED. */
    bool hasActiveTimers() const;

    /** @brief Current countdown snapshot. */
    State getState() const;

    // -- Recents --------------------------------------------------------------

    /** @brief Current recents list (non-owning reference). */
    const std::vector<Timer>& getRecents() const { return mRecents; }

    /** @brief Overwrite recents (capped to kMaxRecents), persist and notify. */
    bool saveRecents(const std::vector<Timer>& list);

private:

    // -- Constants ------------------------------------------------------------

    static constexpr char     skFilePath[] = "timer.json";
    static constexpr uint32_t kNoTimeout   = 0xFFFFFFFF;
    static constexpr size_t   kMaxRecents  = 3;

    // -- State ----------------------------------------------------------------

    const SDK::Kernel& mKernel;
    Callback*          mObserver = nullptr;

    // Active countdown
    TimerState    mState       = TimerState::IDLE;
    uint16_t      mDurationSec = 0;
    Timer::Effect mEffect      = Timer::EFFECT_BEEP_AND_VIBRO;
    uint32_t      mEndTick     = 0;   // valid when RUNNING
    uint32_t      mRemainingMs = 0;   // valid when PAUSED / FIRED

    // Recents
    std::vector<Timer> mRecents;
    char               mBuffer[1024]{};

    inline static constexpr std::array<std::string_view, Timer::EFFECT_COUNT> kEffectJsonKeyValue =
        { "beep_vibro", "vibro", "beep" };

    // -- Helpers --------------------------------------------------------------

    void     arm(uint16_t durationSec, Timer::Effect effect, uint32_t nowMs);

    bool     saveToFile(const std::vector<Timer>& list);
    bool     loadFromFile(std::vector<Timer>& list);
    uint32_t createJSON(const std::vector<Timer>& list, char* buff, uint32_t buffSize);
    bool     parseJSON(char* buff, uint32_t length, std::vector<Timer>& list);
};

#endif // TIMER_MANAGER_HPP
