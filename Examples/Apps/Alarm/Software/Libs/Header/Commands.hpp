
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

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
    // sizeof(AlarmList) = 32 (MessageBase) + kMaxAlarms*sizeof(Alarm) + 2 = 134 bytes
    // -> allocated from Pool 3 (256 bytes), zero dynamic allocations.
    struct AlarmList : public SDK::MessageBase {
        Alarm   alarms[kMaxAlarms];
        uint8_t count;
        bool    timeFormat12h;   // Service -> GUI: true = 12-hour clock
        AlarmList()
            : SDK::MessageBase(ALARM_LIST)
            , alarms{}
            , count(0)
            , timeFormat12h(false)
        {}

        // Service <-> GUI
        //
        // timeFormat12h is only meaningful Service -> GUI; the GUI -> Service
        // direction leaves it at the default (the Service ignores it there).
        explicit AlarmList(const std::vector<Alarm> &list, bool timeFormat12h = false)
            : AlarmList()
        {
            this->count = static_cast<uint8_t>(
                list.size() < kMaxAlarms ? list.size() : kMaxAlarms);
            for (uint8_t i = 0; i < this->count; ++i) {
                this->alarms[i] = list[i];
            }
            this->timeFormat12h = timeFormat12h;
        }
    };

    // Service --> GUI
    struct ActivatedAlarm : public SDK::MessageBase {
        Alarm alarm;
        ActivatedAlarm()
            : SDK::MessageBase(ACTIVATED_ALARM)
            , alarm{}
        {}

        // Service --> GUI
        explicit ActivatedAlarm(const Alarm &alarm)
            : ActivatedAlarm()
        {
            this->alarm = alarm;
        }
    };

    // GUI --> Service
    struct AlarmActivateEffect : public SDK::MessageBase {
        Alarm alarm;
        AlarmActivateEffect()
            : SDK::MessageBase(ACTIVATED_EFFECT)
            , alarm{}
        {}

        // GUI --> Service
        explicit AlarmActivateEffect(const Alarm &alarm)
            : AlarmActivateEffect()
        {
            this->alarm = alarm;
        }
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


} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
