
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstring>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

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
    constexpr SDK::MessageType::Type LAP_END                  = 0x00000007;
    constexpr SDK::MessageType::Type SUMMARY                  = 0x00000008;
    constexpr SDK::MessageType::Type INTERVALS_PHASE_ALERT      = 0x00000009;
    constexpr SDK::MessageType::Type INTERVALS_WORKOUT_COMPLETED = 0x00000010;
    constexpr SDK::MessageType::Type ACCESSORY_STATUS          = 0x00000012;
    constexpr SDK::MessageType::Type WORKOUT_LIST              = 0x00000013;
    constexpr SDK::MessageType::Type WORKOUT_DETAILS           = 0x00000014;
    constexpr SDK::MessageType::Type WORKOUT_DATA              = 0x00000015;
    constexpr SDK::MessageType::Type WORKOUT_STEP              = 0x00000016;

    // GUI --> Service
    constexpr SDK::MessageType::Type SETTINGS_SAVE         = 0x0000000A;
    constexpr SDK::MessageType::Type TRACK_START           = 0x0000000B;
    constexpr SDK::MessageType::Type TRACK_STOP            = 0x0000000C;
    constexpr SDK::MessageType::Type TRACK_PAUSE           = 0x0000000D;
    constexpr SDK::MessageType::Type TRACK_RESUME          = 0x0000000E;
    constexpr SDK::MessageType::Type MANUAL_LAP            = 0x0000000F;
    constexpr SDK::MessageType::Type INTERVALS_NEXT_PHASE  = 0x00000011;  ///< R2: next interval phase or workout step
    constexpr SDK::MessageType::Type WORKOUT_DETAILS_REQUEST = 0x00000017;
    constexpr SDK::MessageType::Type WORKOUT_END             = 0x00000018;

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

    struct GpsFix : public SDK::MessageBase {
        bool state;
        GpsFix()
            : SDK::MessageBase(GPS_FIX)
            , state(false)
        {}

        explicit GpsFix(bool state)
            : GpsFix()
        {
            this->state = state;
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

    /**
     * The structured workouts on the watch, sent when the GUI starts. The
     * list is the service's and stays valid while the app runs: the service
     * builds it only before a run. @p today is the workout planned for today
     * to offer at launch, or kNoToday if there is none or it was completed.
     */
    struct WorkoutList : public SDK::MessageBase {
        static constexpr uint16_t kNoToday = 0xFFFF;

        const SDK::Workout::Library* library;  ///< Non-owning
        uint16_t                     today;

        WorkoutList()
            : SDK::MessageBase(WORKOUT_LIST)
            , library(nullptr)
            , today(kNoToday)
        {}

        WorkoutList(const SDK::Workout::Library* library, uint16_t today)
            : WorkoutList()
        {
            this->library = library;
            this->today   = today;
        }
    };

    /**
     * A workout read in full, in answer to WORKOUT_DETAILS_REQUEST. The
     * program is the service's: it stays valid until the next request or
     * the start of a run. nullptr if the file could not be read.
     */
    struct WorkoutDetails : public SDK::MessageBase {
        uint16_t                     index;    ///< In the list
        const SDK::Workout::Program* program;  ///< Non-owning

        WorkoutDetails()
            : SDK::MessageBase(WORKOUT_DETAILS)
            , index(0)
            , program(nullptr)
        {}

        WorkoutDetails(uint16_t index, const SDK::Workout::Program* program)
            : WorkoutDetails()
        {
            this->index   = index;
            this->program = program;
        }
    };

    /**
     * The workout face, every second while a structured workout runs. The
     * gauge is in m/s for a speed target and bpm for a heart-rate one; arc
     * is where the value sits on the gauge (0-1, with the band in the middle
     * third, faster or higher to the right).
     *
     * Each step is a lap, so the step's average pace and heart rate are the
     * lap's, in Track::Data (avgLapSpeed, avgLapHR).
     */
    struct WorkoutData : public SDK::MessageBase {
        uint32_t remainingMs;  ///< Time step: time left
        uint32_t remainingCm;  ///< Distance step: distance left
        uint32_t stepTimeMs;   ///< Time in the step so far
        uint32_t rep;          ///< Pass of the innermost repeat; 0 outside one
        uint32_t reps;         ///< Its passes in all; 0 if unlimited
        float    value;        ///< Gauge value; 0 with no target, or a speed too low for a pace
        float    low;          ///< Target band
        float    high;
        float    arc;
        uint8_t  stepEnd;      ///< SDK::Workout::StepEnd
        uint8_t  target;       ///< SDK::Workout::Target
        uint8_t  zone;         ///< SDK::Workout::Zone
        uint8_t  intensity;    ///< SDK::Workout::Intensity
        bool     leadIn;       ///< The step ends within about 5 s

        WorkoutData()
            : SDK::MessageBase(WORKOUT_DATA)
            , remainingMs(0)
            , remainingCm(0)
            , stepTimeMs(0)
            , rep(0)
            , reps(0)
            , value(0.0f)
            , low(0.0f)
            , high(0.0f)
            , arc(0.0f)
            , stepEnd(0)
            , target(0)
            , zone(0)
            , intensity(0)
            , leadIn(false)
        {}
    };

    /**
     * A workout step, sent in pairs when a workout starts and at each step
     * change: first the step that has just started (for the next-step card
     * and the step face), then the one after it (next set, for the
     * next-step face; none set if nothing follows).
     */
    struct WorkoutStep : public SDK::MessageBase {
        char     notes[SDK::Workout::kStepNotesBytes];  ///< The step's notes, or else its name; UTF-8, may be empty
        char     duration[24];  ///< e.g. "400 m"; empty for an open step
        char     target[32];    ///< e.g. "3:45-4:05 /km"; empty for no target
        uint32_t rep;           ///< The step that started: pass of the innermost repeat; 0 outside one
        uint32_t reps;          ///< Its passes in all; 0 if unlimited
        uint8_t  intensity;     ///< SDK::Workout::Intensity
        bool     next;          ///< The step after the current one
        bool     none;          ///< next only: nothing follows

        WorkoutStep()
            : SDK::MessageBase(WORKOUT_STEP)
            , notes{}
            , duration{}
            , target{}
            , rep(0)
            , reps(0)
            , intensity(0)
            , next(false)
            , none(false)
        {}
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
        static constexpr int16_t kNoWorkout = -1;

        bool    intervalsMode = false;
        int16_t workout       = kNoWorkout;  ///< Index in the workout list to follow; wins over intervalsMode
        TrackStart() : SDK::MessageBase(TRACK_START) {}

        explicit TrackStart(bool intervalsMode, int16_t workout = kNoWorkout)
            : TrackStart()
        {
            this->intervalsMode = intervalsMode;
            this->workout       = workout;
        }
    };

    /// Read a workout from the list in full, for its details screen.
    struct WorkoutDetailsRequest : public SDK::MessageBase {
        uint16_t index;
        WorkoutDetailsRequest() : SDK::MessageBase(WORKOUT_DETAILS_REQUEST), index(0) {}

        explicit WorkoutDetailsRequest(uint16_t index)
            : WorkoutDetailsRequest()
        {
            this->index = index;
        }
    };

    /// End workout, from the action menu: the run carries on as a free run.
    struct WorkoutEnd : public SDK::MessageBase {
        WorkoutEnd() : SDK::MessageBase(WORKOUT_END) {}
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


    // The kernel's largest message block is 256 bytes.
    static_assert(sizeof(WorkoutData) <= 256, "WorkoutData must fit a message block");
    static_assert(sizeof(WorkoutStep) <= 256, "WorkoutStep must fit a message block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
