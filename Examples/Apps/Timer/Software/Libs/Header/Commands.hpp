
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

    // Maximum number of timers that can be transferred in a single message.
    // Must match or exceed TimerManager::kInitialCount.
    static constexpr size_t kMaxTimers = 20;

    // Application custom commands

    // Service <-> GUI
    constexpr SDK::MessageType::Type TIMER_LIST        = 0x00000001;

    // Service --> GUI
    constexpr SDK::MessageType::Type ACTIVATED_TIMER   = 0x00000002;

    // GUI --> Service
    constexpr SDK::MessageType::Type ACTIVATED_EFFECT  = 0x00000003;
    constexpr SDK::MessageType::Type TIMER_STOP        = 0x00000004;
    constexpr SDK::MessageType::Type TIMER_STOP_ALL    = 0x00000005;
    constexpr SDK::MessageType::Type TIMER_SNOOZE      = 0x00000006;
    constexpr SDK::MessageType::Type TIMER_SNOOZE_ALL  = 0x00000007;

    // Service <-> GUI
    //
    // Fixed-size array avoids heap allocation in the message pool path.
    // sizeof(TimerList) = 32 (MessageBase) + kMaxTimers*sizeof(Timer) + 2 = 134 bytes
    // -> allocated from Pool 3 (256 bytes), zero dynamic allocations.
    struct TimerList : public SDK::MessageBase {
        Timer   timers[kMaxTimers];
        uint8_t count;
        bool    timeFormat12h;   // Service -> GUI: true = 12-hour clock
        TimerList()
            : SDK::MessageBase(TIMER_LIST)
            , timers{}
            , count(0)
            , timeFormat12h(false)
        {}
    };

    // Service --> GUI
    struct ActivatedTimer : public SDK::MessageBase {
        Timer timer;
        ActivatedTimer()
            : SDK::MessageBase(ACTIVATED_TIMER)
            , timer{}
        {}
    };

    // GUI --> Service
    struct TimerActivateEffect : public SDK::MessageBase {
        Timer timer;
        TimerActivateEffect()
            : SDK::MessageBase(ACTIVATED_EFFECT)
            , timer{}
        {}
    };

    struct TimerStop : public SDK::MessageBase {
        Timer timer;
        TimerStop()
            : SDK::MessageBase(TIMER_STOP)
            , timer{}
        {}
    };

    struct TimerStopAll : public SDK::MessageBase {
        TimerStopAll()
            : SDK::MessageBase(TIMER_STOP_ALL)
        {}
    };

    struct TimerSnooze : public SDK::MessageBase {
        Timer timer;
        TimerSnooze()
            : SDK::MessageBase(TIMER_SNOOZE)
            , timer{}
        {}
    };

    struct TimerSnoozeAll : public SDK::MessageBase {
        TimerSnoozeAll()
            : SDK::MessageBase(TIMER_SNOOZE_ALL)
        {}
    };


// Helper wrapper
class Sender {
public:
    Sender(const SDK::Kernel &kernel) :
            mKernel(kernel)
    {
    }
    virtual ~Sender() = default;

    // Service <-> GUI
    //
    // timeFormat12h is only meaningful Service -> GUI; the GUI -> Service
    // direction leaves it at the default (the Service ignores it there).
    bool listUpd(const std::vector<Timer> &list, bool timeFormat12h = false)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerList>();
        if (msg) {
            msg->count = static_cast<uint8_t>(
                list.size() < kMaxTimers ? list.size() : kMaxTimers);
            for (uint8_t i = 0; i < msg->count; ++i) {
                msg->timers[i] = list[i];
            }
            msg->timeFormat12h = timeFormat12h;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // Service --> GUI
    bool timerActivated(const Timer &timer)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::ActivatedTimer>();
        if (msg) {
            msg->timer = timer;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // GUI --> Service
    bool activateEffect(const Timer &timer)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerActivateEffect>();
        if (msg) {
            msg->timer = timer;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool stop(const Timer &timer)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerStop>();
        if (msg) {
            msg->timer = timer;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool stopAll()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerStopAll>();
        if (msg) {
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool snooze(const Timer &timer)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerSnooze>();
        if (msg) {
            msg->timer = timer;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool snoozeAll()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TimerSnoozeAll>();
        if (msg) {
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
