
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstring>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

// Application types
#include "Settings.hpp"
#include "Track.hpp"
#include "ActivitySummary.hpp"
#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"  // Config::kBinCount

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
    constexpr SDK::MessageType::Type TRACK_STATE_UPDATE = 0x00000005;
    constexpr SDK::MessageType::Type TRACK_DATA_UPDATE  = 0x00000006;
    constexpr SDK::MessageType::Type LAP_END                  = 0x00000007;
    constexpr SDK::MessageType::Type SUMMARY                  = 0x00000008;
    constexpr SDK::MessageType::Type INTERVALS_PHASE_ALERT      = 0x00000009;
    constexpr SDK::MessageType::Type INTERVALS_WORKOUT_COMPLETED = 0x00000010;
    constexpr SDK::MessageType::Type CALIB_DATA                 = 0x00000013;
    // 0x12 is TRACK_CALIBRATE here (Running used 0x12 for ACCESSORY_STATUS); the
    // first free Service-->GUI id past Treadmill's existing 0x12..0x15 is 0x16.
    constexpr SDK::MessageType::Type ACCESSORY_STATUS          = 0x00000016;

    // GUI --> Service
    constexpr SDK::MessageType::Type SETTINGS_SAVE         = 0x0000000A;
    constexpr SDK::MessageType::Type TRACK_START           = 0x0000000B;
    constexpr SDK::MessageType::Type TRACK_STOP            = 0x0000000C;
    constexpr SDK::MessageType::Type TRACK_PAUSE           = 0x0000000D;
    constexpr SDK::MessageType::Type TRACK_RESUME          = 0x0000000E;
    constexpr SDK::MessageType::Type MANUAL_LAP            = 0x0000000F;
    constexpr SDK::MessageType::Type INTERVALS_NEXT_PHASE  = 0x00000011;
    constexpr SDK::MessageType::Type TRACK_CALIBRATE       = 0x00000012;
    constexpr SDK::MessageType::Type CALIB_DATA_REQUEST    = 0x00000014;
    constexpr SDK::MessageType::Type CALIB_CLEAR           = 0x00000015;

    // Service <-> GUI
    struct SettingsUpd : public SDK::MessageBase {
        // Application settings
        Settings settings;

        // Kernel settings
        bool    unitsImperial;
        bool    timeFormat12h;   // true = 12-hour clock, false = 24-hour
        uint8_t hrThresholds[kHrThresholdsCount];
        uint8_t hrThresholdsCount;

        SettingsUpd()
            : SDK::MessageBase(SETTINGS_UPDATE)
            , unitsImperial(false)
            , timeFormat12h(false)
            , hrThresholds {}
            , hrThresholdsCount(0)
        {}

        explicit SettingsUpd(Settings settings, bool units, bool timeFormat12h,
                         const uint8_t (&thresholds)[kHrThresholdsCount], uint8_t thresholdCount)
            : SettingsUpd()
        {
            this->settings          = settings;
            this->unitsImperial     = units;
            this->timeFormat12h     = timeFormat12h;
            memcpy(this->hrThresholds, thresholds, sizeof(this->hrThresholds));
            this->hrThresholdsCount = thresholdCount;
        }
    };

    // Service --> GUI
    struct Time : public SDK::MessageBase {
        std::tm localTime;
        Time()
            : SDK::MessageBase(LOCAL_TIME)
            , localTime {}
        {}

        explicit Time(std::tm localTime)
            : Time()
        {
            this->localTime = localTime;
        }
    };

    struct Battery : public SDK::MessageBase {
        uint8_t level;
        Battery()
            : SDK::MessageBase(BATTERY)
            , level(0)
        {}

        explicit Battery(uint8_t level)
            : Battery()
        {
            this->level = level;
        }
    };

    struct TrackStateUpd : public SDK::MessageBase {
        Track::State state;
        TrackStateUpd()
            : SDK::MessageBase(TRACK_STATE_UPDATE)
            , state{}
        {}

        explicit TrackStateUpd(Track::State state)
            : TrackStateUpd()
        {
            this->state = state;
        }
    };

    struct TrackDataUpd : public SDK::MessageBase {
        Track::Data data;
        TrackDataUpd()
            : SDK::MessageBase(TRACK_DATA_UPDATE)
            , data{}
        {}

        explicit TrackDataUpd(const Track::Data &data)
            : TrackDataUpd()
        {
            this->data = data;
        }
    };

    struct LapEnded : public SDK::MessageBase {
        uint32_t lapNum;
        LapEnded()
            : SDK::MessageBase(LAP_END)
            , lapNum(0)
        {}

        explicit LapEnded(uint32_t lapNum)
            : LapEnded()
        {
            this->lapNum = lapNum;
        }
    };

    struct Summary : public SDK::MessageBase {
        const ActivitySummary* summary; ///< Non-owning pointer; receiver must copy before releaseMessage
        Summary()
            : SDK::MessageBase(SUMMARY)
            , summary(nullptr)
        {}

        explicit Summary(const ActivitySummary* summaryPtr)
            : Summary()
        {
            this->summary = summaryPtr;
        }
    };

    struct IntervalsPhaseAlert : public SDK::MessageBase {
        Track::IntervalsData intervals; ///< Snapshot of the NEW phase — already set before this message is sent
        IntervalsPhaseAlert() : SDK::MessageBase(INTERVALS_PHASE_ALERT) {}

        explicit IntervalsPhaseAlert(const Track::IntervalsData& intervals)
            : IntervalsPhaseAlert()
        {
            this->intervals = intervals;
        }
    };

    struct IntervalsWorkoutCompleted : public SDK::MessageBase {
        IntervalsWorkoutCompleted() : SDK::MessageBase(INTERVALS_WORKOUT_COMPLETED) {}
    };

    // External-accessory link status forwarded from the kernel's
    // EVENT_ACCESSORY_STATUS, for the pre-activity HR indicator (WP-S4).
    struct AccessoryStatusUpd : public SDK::MessageBase {
        uint8_t state;     ///< SDK::Accessory::State
        char    name[24];  ///< device name (may be empty)
        AccessoryStatusUpd()
            : SDK::MessageBase(ACCESSORY_STATUS)
            , state(0)
            , name{}
        {}

        explicit AccessoryStatusUpd(uint8_t state, const char* name)
            : AccessoryStatusUpd()
        {
            this->state = state;
            if (name) {
                strncpy(this->name, name, sizeof(this->name) - 1);
            }
        }
    };

    // Calibration snapshot for Settings > Calibration > View Data. The Service
    // owns all access to the on-disk stride LUT; this is its read-only summary.
    struct CalibData : public SDK::MessageBase {
        uint8_t  status;          ///< 0 uncalibrated / 1 estimating / 2 calibrated
        uint16_t validBins;
        uint16_t binCount;
        float    totalDistanceM;
        uint8_t  pct[SDK::Calibration::Config::kBinCount];  ///< per-bin fill toward validity, 0..100
        CalibData()
            : SDK::MessageBase(CALIB_DATA)
            , status(0), validBins(0), binCount(0), totalDistanceM(0.0f), pct{}
        {}

        explicit CalibData(uint8_t status, uint16_t validBins, uint16_t binCount,
                         float totalDistanceM, const uint8_t* pct, uint16_t pctCount)
            : CalibData()
        {
            this->status         = status;
            this->validBins      = validBins;
            this->binCount       = binCount;
            this->totalDistanceM = totalDistanceM;
            const uint16_t n = (pctCount < SDK::Calibration::Config::kBinCount)
                ? pctCount : SDK::Calibration::Config::kBinCount;
            memcpy(this->pct, pct, n);
        }
    };

    // GUI --> Service
    struct SettingsSave : public SDK::MessageBase {
        // Application settings
        Settings settings;

        SettingsSave()
            : SDK::MessageBase(SETTINGS_SAVE)
        {}

        explicit SettingsSave(Settings settings)
            : SettingsSave()
        {
            this->settings = settings;
        }
    };

    struct TrackStart : public SDK::MessageBase {
        bool intervalsMode = false;
        TrackStart() : SDK::MessageBase(TRACK_START) {}

        explicit TrackStart(bool intervalsMode)
            : TrackStart()
        {
            this->intervalsMode = intervalsMode;
        }
    };

    struct IntervalsNextPhase : public SDK::MessageBase {
        IntervalsNextPhase() : SDK::MessageBase(INTERVALS_NEXT_PHASE) {}
    };

    struct TrackStop : public SDK::MessageBase {
        bool discard;   // If true, tarck will be discarded, otherwise saved
        TrackStop()
            : SDK::MessageBase(TRACK_STOP)
            , discard(false)
        {}

        explicit TrackStop(bool discard)
            : TrackStop()
        {
            this->discard = discard;
        }
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

    // Post-run "Calibrate & Save" (§5): the user-entered actual treadmill
    // distance, in metres (the GUI converts from display units). A value <= 0
    // means "skip" — finalise with the estimated distance.
    struct TrackCalibrate : public SDK::MessageBase {
        float distanceActualM;
        TrackCalibrate()
            : SDK::MessageBase(TRACK_CALIBRATE)
            , distanceActualM(0.0f)
        {}

        explicit TrackCalibrate(float distanceActualM)
            : TrackCalibrate()
        {
            this->distanceActualM = distanceActualM;
        }
    };

    // Ask the Service for a fresh calibration snapshot (-> CalibData).
    struct CalibDataRequest : public SDK::MessageBase {
        CalibDataRequest() : SDK::MessageBase(CALIB_DATA_REQUEST) {}
    };

    // Ask the Service to back up and clear the calibration stores.
    struct CalibClear : public SDK::MessageBase {
        CalibClear() : SDK::MessageBase(CALIB_CLEAR) {}
    };

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
