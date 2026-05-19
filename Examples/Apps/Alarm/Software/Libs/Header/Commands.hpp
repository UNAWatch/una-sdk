
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Alarm.hpp"
#include <vector>
#include <cstddef>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Maximum number of alarms that can be transferred in a single message.
    // Must match or exceed AlarmManager::kInitialCount.
    static constexpr size_t kMaxAlarms = 20;

    // Application custom commands

    // Service <-> GUI
    constexpr SDK::MessageType::Type ALARM_LIST        = 0x00000001;

    // Service --> GUI
    constexpr SDK::MessageType::Type ACTIVATED_ALARM   = 0x00000002;

    // GUI --> Service
    constexpr SDK::MessageType::Type ACTIVATED_EFFECT  = 0x00000003;
    constexpr SDK::MessageType::Type ALARM_STOP        = 0x00000004;
    constexpr SDK::MessageType::Type ALARM_STOP_ALL    = 0x00000005;
    constexpr SDK::MessageType::Type ALARM_SNOOZE      = 0x00000006;
    constexpr SDK::MessageType::Type ALARM_SNOOZE_ALL  = 0x00000007;

    // Service <-> GUI
    //
    // Fixed-size array avoids heap allocation in the message pool path.
    // sizeof(AlarmList) = 32 (MessageBase) + kMaxAlarms*sizeof(Alarm) + 1 = 133 bytes
    // -> allocated from Pool 3 (256 bytes), zero dynamic allocations.
    struct AlarmList : public SDK::MessageBase {
        Alarm   alarms[kMaxAlarms];
        uint8_t count;
        AlarmList()
            : SDK::MessageBase(ALARM_LIST)
            , alarms{}
            , count(0)
        {}
    };

    // Service --> GUI
    struct ActivatedAlarm : public SDK::MessageBase {
        Alarm alarm;
        ActivatedAlarm()
            : SDK::MessageBase(ACTIVATED_ALARM)
            , alarm{}
        {}
    };

    // GUI --> Service
    struct AlarmActivateEffect : public SDK::MessageBase {
        Alarm alarm;
        AlarmActivateEffect()
            : SDK::MessageBase(ACTIVATED_EFFECT)
            , alarm{}
        {}
    };

    struct AlarmStop : public SDK::MessageBase {
        Alarm alarm;
        AlarmStop()
            : SDK::MessageBase(ALARM_STOP)
            , alarm{}
        {}
    };

    struct AlarmStopAll : public SDK::MessageBase {
        AlarmStopAll()
            : SDK::MessageBase(ALARM_STOP_ALL)
        {}
    };

    struct AlarmSnooze : public SDK::MessageBase {
        Alarm alarm;
        AlarmSnooze()
            : SDK::MessageBase(ALARM_SNOOZE)
            , alarm{}
        {}
    };

    struct AlarmSnoozeAll : public SDK::MessageBase {
        AlarmSnoozeAll()
            : SDK::MessageBase(ALARM_SNOOZE_ALL)
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
    bool listUpd(const std::vector<Alarm> &list)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmList>();
        if (msg) {
            msg->count = static_cast<uint8_t>(
                list.size() < kMaxAlarms ? list.size() : kMaxAlarms);
            for (uint8_t i = 0; i < msg->count; ++i) {
                msg->alarms[i] = list[i];
            }
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // Service --> GUI
    bool alarmActivated(const Alarm &alarm)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::ActivatedAlarm>();
        if (msg) {
            msg->alarm = alarm;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // GUI --> Service
    bool activateEffect(const Alarm &alarm)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmActivateEffect>();
        if (msg) {
            msg->alarm = alarm;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool stop(const Alarm &alarm)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmStop>();
        if (msg) {
            msg->alarm = alarm;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool stopAll()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmStopAll>();
        if (msg) {
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool snooze(const Alarm &alarm)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmSnooze>();
        if (msg) {
            msg->alarm = alarm;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool snoozeAll()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::AlarmSnoozeAll>();
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
