
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstring>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Kernel/Kernel.hpp"

// Application types
#include "Settings.hpp"
#include "Track.hpp"
#include "ActivitySummary.hpp"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Kernel HR configuration — shared defaults used before first kernel update
    static constexpr uint8_t kHrThresholdsCount                       = 6;
    static constexpr uint8_t kHrThresholdsDefault[kHrThresholdsCount] = { 95, 114, 133, 152, 171, 190 };

    // Application custom commands
    // Service --> GUI
    constexpr SDK::MessageType::Type SETTINGS_UPDATE    = 0x00000001;
    constexpr SDK::MessageType::Type LOCAL_TIME         = 0x00000002;
    constexpr SDK::MessageType::Type BATTERY            = 0x00000003;
    constexpr SDK::MessageType::Type GPS_FIX            = 0x00000004;
    constexpr SDK::MessageType::Type TRACK_STATE_UPDATE = 0x00000005;
    constexpr SDK::MessageType::Type TRACK_DATA_UPDATE  = 0x00000006;
    constexpr SDK::MessageType::Type LAP_END            = 0x00000007;
    constexpr SDK::MessageType::Type SUMMARY            = 0x00000008;

    // GUI --> Service
    constexpr SDK::MessageType::Type SETTINGS_SAVE      = 0x0000000A;
    constexpr SDK::MessageType::Type TRACK_START        = 0x0000000B;
    constexpr SDK::MessageType::Type TRACK_STOP         = 0x0000000C;
    constexpr SDK::MessageType::Type TRACK_PAUSE        = 0x0000000D;
    constexpr SDK::MessageType::Type TRACK_RESUME       = 0x0000000E;
    constexpr SDK::MessageType::Type MANUAL_LAP         = 0x0000000F;

    // Service <-> GUI
    struct SettingsUpd : public SDK::MessageBase {
        // Application settings
        Settings settings;

        // Kernel settings
        bool    unitsImperial;
        uint8_t hrThresholds[kHrThresholdsCount];
        uint8_t hrThresholdsCount;

        SettingsUpd()
            : SDK::MessageBase(SETTINGS_UPDATE)
            , unitsImperial(false)
            , hrThresholds {}
            , hrThresholdsCount(0)
        {}
    };

    // Service --> GUI
    struct Time : public SDK::MessageBase {
        std::tm localTime;
        Time()
            : SDK::MessageBase(LOCAL_TIME)
            , localTime {}
        {}
    };

    struct Battery : public SDK::MessageBase {
        uint8_t level;
        Battery()
            : SDK::MessageBase(BATTERY)
            , level(0)
        {}
    };

    struct GpsFix : public SDK::MessageBase {
        bool state;
        GpsFix()
            : SDK::MessageBase(GPS_FIX)
            , state(false)
        {}
    };

    struct TrackStateUpd : public SDK::MessageBase {
        Track::State state;
        TrackStateUpd()
            : SDK::MessageBase(TRACK_STATE_UPDATE)
            , state{}
        {}
    };

    struct TrackDataUpd : public SDK::MessageBase {
        Track::Data data;
        TrackDataUpd()
            : SDK::MessageBase(TRACK_DATA_UPDATE)
            , data{}
        {}
    };

    struct LapEnded : public SDK::MessageBase {
        uint32_t lapNum;
        LapEnded()
            : SDK::MessageBase(LAP_END)
            , lapNum(0)
        {}
    };

    struct Summary : public SDK::MessageBase {
        const ActivitySummary* summary; ///< Non-owning pointer; receiver must copy before releaseMessage
        Summary()
            : SDK::MessageBase(SUMMARY)
            , summary(nullptr)
        {}
    };

    // GUI --> Service
    struct SettingsSave : public SDK::MessageBase {
        // Application settings
        Settings settings;

        SettingsSave()
            : SDK::MessageBase(SETTINGS_SAVE)
        {}
    };

    struct TrackStart : public SDK::MessageBase {
        TrackStart() : SDK::MessageBase(TRACK_START) {}
    };

    struct TrackStop : public SDK::MessageBase {
        bool discard;   // If true, tarck will be discarded, otherwise saved
        TrackStop()
            : SDK::MessageBase(TRACK_STOP)
            , discard(false)
        {}
    };

    struct TrackPause : public SDK::MessageBase {
        TrackPause() : SDK::MessageBase(TRACK_PAUSE) {}
    };

    struct TrackResume : public SDK::MessageBase {
        TrackResume() : SDK::MessageBase(TRACK_RESUME) {}
    };

    struct ManualLap : public SDK::MessageBase {
        ManualLap() : SDK::MessageBase(MANUAL_LAP) {}
    };


// Helper wrapper
class Sender {
public:
    Sender(const SDK::Kernel &kernel) :
            mKernel(kernel)
    {
    }
    virtual ~Sender() = default;

    // Service --> GUI
    bool settingsUpd(Settings settings, bool units,
                     const uint8_t (&thresholds)[kHrThresholdsCount], uint8_t thresholdCount)
    {
        if (auto msg = SDK::make_msg<CustomMessage::SettingsUpd>(mKernel)) {
            msg->settings          = settings;
            msg->unitsImperial     = units;
            memcpy(msg->hrThresholds, thresholds, sizeof(msg->hrThresholds));
            msg->hrThresholdsCount = thresholdCount;
            return msg.send();
        }

        return false;
    }

    bool time(std::tm localTime)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::Time>();
        if (msg) {
            msg->localTime = localTime;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool battery(uint8_t level)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::Battery>();
        if (msg) {
            msg->level = level;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool fix(bool state)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::GpsFix>();
        if (msg) {
            msg->state = state;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackState(Track::State state)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackStateUpd>();
        if (msg) {
            msg->state = state;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackData(const Track::Data &data)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackDataUpd>();
        if (msg) {
            msg->data = data;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool lapEnd(uint32_t lapNum)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::LapEnded>();
        if (msg) {
            msg->lapNum = lapNum;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool summary(const ActivitySummary* summaryPtr)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::Summary>();
        if (msg) {
            msg->summary = summaryPtr;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    // GUI --> Service
    bool settingsSave(Settings settings)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::SettingsSave>();
        if (msg) {
            msg->settings = settings;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackStart()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackStart>();
        if (msg) {
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackStop(bool discard)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackStop>();
        if (msg) {
            msg->discard = discard;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackPause()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackPause>();
        if (msg) {
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool trackResume()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::TrackResume>();
        if (msg) {
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

    bool manualLap()
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::ManualLap>();
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
