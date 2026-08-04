#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Timer.hpp"
#include <vector>
#include <cstddef>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Maximum number of recent timers kept and transferred in one message.
    static constexpr size_t kMaxRecents = Timer::kMaxRecents;   // single source: Timer.hpp

    // -- Message type ids (application-custom) --------------------------------

    // GUI --> Service
    constexpr SDK::MessageType::Type TIMER_START        = 0x00000001;
    constexpr SDK::MessageType::Type TIMER_CONTROL      = 0x00000002;
    constexpr SDK::MessageType::Type TIMER_RECENTS_SAVE = 0x00000003;

    // Service --> GUI
    constexpr SDK::MessageType::Type TIMER_STATE        = 0x00000010;
    constexpr SDK::MessageType::Type TIMER_FIRED        = 0x00000011;
    constexpr SDK::MessageType::Type TIMER_RECENTS      = 0x00000012;

    // -- Payload helpers ------------------------------------------------------

    /** @brief One recent-timer entry (duration + effect), packed for transfer. */
    struct RecentEntry {
        uint16_t      durationSec;
        Timer::Effect effect;
    };

    /**
     * @brief Copy up to kMaxRecents timers into a packed RecentEntry array.
     * @return The number actually copied, for the message's count field.
     *
     * Shared by the two recents messages, which carry the same payload in
     * opposite directions.
     */
    inline uint8_t packRecents(RecentEntry (&entries)[kMaxRecents],
                               const std::vector<Timer>& list)
    {
        const uint8_t count = static_cast<uint8_t>(
            list.size() < kMaxRecents ? list.size() : kMaxRecents);
        for (uint8_t i = 0; i < count; ++i) {
            entries[i].durationSec = list[i].durationSec;
            entries[i].effect      = list[i].effect;
        }
        return count;
    }

    /** @brief Sub-command carried by a TIMER_CONTROL message. */
    enum class TimerCmd : uint8_t {
        PAUSE,
        RESUME,
        RESET,
        STOP,
        REPEAT,
        REPLAY_ALERT   ///< Re-play the fired alert effect (periodic re-indication).
    };

    // -- GUI --> Service ------------------------------------------------------

    struct TimerStart : public SDK::MessageBase {
        uint16_t      durationSec;
        Timer::Effect effect;
        TimerStart()
            : SDK::MessageBase(TIMER_START)
            , durationSec(0)
            , effect(Timer::EFFECT_BEEP_AND_VIBRO)
        {}

        TimerStart(uint16_t durationSec, Timer::Effect effect)
            : TimerStart()
        {
            this->durationSec = durationSec;
            this->effect      = effect;
        }
    };

    struct TimerControl : public SDK::MessageBase {
        TimerCmd cmd;
        TimerControl()
            : SDK::MessageBase(TIMER_CONTROL)
            , cmd(TimerCmd::STOP)
        {}

        explicit TimerControl(TimerCmd cmd)
            : TimerControl()
        {
            this->cmd = cmd;
        }
    };

    struct TimerRecentsSave : public SDK::MessageBase {
        RecentEntry entries[kMaxRecents];
        uint8_t     count;
        TimerRecentsSave()
            : SDK::MessageBase(TIMER_RECENTS_SAVE)
            , entries{}
            , count(0)
        {}

        explicit TimerRecentsSave(const std::vector<Timer>& list)
            : TimerRecentsSave()
        {
            this->count = packRecents(this->entries, list);
        }
    };

    // -- Service --> GUI ------------------------------------------------------

    // Live countdown snapshot. The GUI extrapolates the displayed time locally
    // from the shared monotonic tick:
    //   RUNNING -> remaining = endTick - now
    //   PAUSED  -> remaining = remainingMs
    struct TimerStateMsg : public SDK::MessageBase {
        uint8_t       state;        // TimerState
        uint32_t      endTick;      // absolute tick of expiry (RUNNING)
        uint32_t      remainingMs;  // frozen remainder (PAUSED)
        uint16_t      durationSec;
        Timer::Effect effect;
        TimerStateMsg()
            : SDK::MessageBase(TIMER_STATE)
            , state(static_cast<uint8_t>(TimerState::IDLE))
            , endTick(0)
            , remainingMs(0)
            , durationSec(0)
            , effect(Timer::EFFECT_BEEP_AND_VIBRO)
        {}

        TimerStateMsg(TimerState state, uint32_t endTick, uint32_t remainingMs,
                      uint16_t durationSec, Timer::Effect effect)
            : TimerStateMsg()
        {
            this->state       = static_cast<uint8_t>(state);
            this->endTick     = endTick;
            this->remainingMs = remainingMs;
            this->durationSec = durationSec;
            this->effect      = effect;
        }
    };

    struct TimerFired : public SDK::MessageBase {
        uint16_t      durationSec;
        Timer::Effect effect;
        bool          background;   ///< true = fired while the GUI was closed.
        TimerFired()
            : SDK::MessageBase(TIMER_FIRED)
            , durationSec(0)
            , effect(Timer::EFFECT_BEEP_AND_VIBRO)
            , background(false)
        {}

        TimerFired(const Timer& timer, bool background)
            : TimerFired()
        {
            this->durationSec = timer.durationSec;
            this->effect      = timer.effect;
            this->background  = background;
        }
    };

    struct TimerRecents : public SDK::MessageBase {
        RecentEntry entries[kMaxRecents];
        uint8_t     count;
        TimerRecents()
            : SDK::MessageBase(TIMER_RECENTS)
            , entries{}
            , count(0)
        {}

        explicit TimerRecents(const std::vector<Timer>& list)
            : TimerRecents()
        {
            this->count = packRecents(this->entries, list);
        }
    };


} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
