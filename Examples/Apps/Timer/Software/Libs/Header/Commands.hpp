#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Timer.hpp"
#include <vector>
#include <cstddef>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Maximum number of recent timers kept and transferred in one message.
    static constexpr size_t kMaxRecents = 3;

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

    /** @brief Sub-command carried by a TIMER_CONTROL message. */
    enum class TimerCmd : uint8_t {
        PAUSE,
        RESUME,
        RESET,
        STOP,
        REPEAT
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
    };

    struct TimerControl : public SDK::MessageBase {
        TimerCmd cmd;
        TimerControl()
            : SDK::MessageBase(TIMER_CONTROL)
            , cmd(TimerCmd::STOP)
        {}
    };

    struct TimerRecentsSave : public SDK::MessageBase {
        RecentEntry entries[kMaxRecents];
        uint8_t     count;
        TimerRecentsSave()
            : SDK::MessageBase(TIMER_RECENTS_SAVE)
            , entries{}
            , count(0)
        {}
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
    };

    struct TimerRecents : public SDK::MessageBase {
        RecentEntry entries[kMaxRecents];
        uint8_t     count;
        TimerRecents()
            : SDK::MessageBase(TIMER_RECENTS)
            , entries{}
            , count(0)
        {}
    };


// Helper wrapper: constructs, sends and releases the pool message in one call.
class Sender {
public:
    Sender(const SDK::Kernel &kernel) :
            mKernel(kernel)
    {
    }
    virtual ~Sender() = default;

    // -- GUI --> Service --------------------------------------------------

    bool start(uint16_t durationSec, Timer::Effect effect)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerStart>();
        if (msg) {
            msg->durationSec = durationSec;
            msg->effect      = effect;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool control(TimerCmd cmd)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerControl>();
        if (msg) {
            msg->cmd = cmd;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool pause()  { return control(TimerCmd::PAUSE);  }
    bool resume() { return control(TimerCmd::RESUME); }
    bool reset()  { return control(TimerCmd::RESET);  }
    bool stop()   { return control(TimerCmd::STOP);   }
    bool repeat() { return control(TimerCmd::REPEAT); }

    bool saveRecents(const std::vector<Timer>& list)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerRecentsSave>();
        if (msg) {
            msg->count = static_cast<uint8_t>(
                list.size() < kMaxRecents ? list.size() : kMaxRecents);
            for (uint8_t i = 0; i < msg->count; ++i) {
                msg->entries[i].durationSec = list[i].durationSec;
                msg->entries[i].effect      = list[i].effect;
            }
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // -- Service --> GUI --------------------------------------------------

    bool sendState(TimerState state, uint32_t endTick, uint32_t remainingMs,
                   uint16_t durationSec, Timer::Effect effect)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerStateMsg>();
        if (msg) {
            msg->state       = static_cast<uint8_t>(state);
            msg->endTick     = endTick;
            msg->remainingMs = remainingMs;
            msg->durationSec = durationSec;
            msg->effect      = effect;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool fired(const Timer& timer, bool background)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerFired>();
        if (msg) {
            msg->durationSec = timer.durationSec;
            msg->effect      = timer.effect;
            msg->background  = background;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool sendRecents(const std::vector<Timer>& list)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<TimerRecents>();
        if (msg) {
            msg->count = static_cast<uint8_t>(
                list.size() < kMaxRecents ? list.size() : kMaxRecents);
            for (uint8_t i = 0; i < msg->count; ++i) {
                msg->entries[i].durationSec = list[i].durationSec;
                msg->entries[i].effect      = list[i].effect;
            }
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

private:
    const SDK::Kernel &mKernel;
};


} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
