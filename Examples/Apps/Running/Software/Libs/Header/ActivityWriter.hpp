/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @date    31-08-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes activity data to a FIT file.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef ACTIVITY_WRITER_HPP
#define ACTIVITY_WRITER_HPP

#include <cstdint>
#include <cstdbool>
#include <string>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/FitHelper/FitHelper.hpp"

extern "C" {
#include "fit_example.h"
}

/**
 * @class ActivityWriter
 * @brief Serializes activity data to a FIT file.
 */
class ActivityWriter {
public:
    struct AppInfo {
        std::time_t timestamp  = 0;  // UTC
        uint32_t    appVersion = 0;  // Application version 4 bytes LE [patch, minor, major, 0]
        std::string devID;           // Developer ID (max len 16)
        std::string appID;           // Application ID (max len 16)
    };

    struct RecordData {
        enum class Field : uint8_t {
            COORDS     = 1u << 0, // lat/long valid as a group
            SPEED      = 1u << 1,
            ALTITUDE   = 1u << 2,
            HEART_RATE   = 1u << 3,
            BATTERY      = 1u << 4,
            CADENCE      = 1u << 5,
            STEP_LENGTH  = 1u << 6,
        };

        void set(Field f)                 { mFlags |= mask(f); }
        void clear(Field f)               { mFlags &= static_cast<uint8_t>(~mask(f)); }
        void set(Field f, bool state)     { state ? set(f) : clear(f); }
        bool has(Field f) const           { return (mFlags & mask(f)) != 0; }
        void clearAll()                   { mFlags = 0; }

        std::time_t timestamp      = 0;     // UTC
        float       latitude       = 0.0f;  // degrees
        float       longitude      = 0.0f;  // degrees
        float       speed          = 0.0f;  // m/s
        float       altitude       = 0.0f;  // m
        float       heartRate      = 0.0f;  // bpm
        uint8_t     batteryLevel   = 0;     // %
        uint16_t    batteryVoltage = 0;     // mV
        float       cadenceSpm     = 0.0f;  // steps/min
        float       stepLengthM    = 0.0f;  // single-step distance, m

    private:
        static constexpr uint8_t mask(Field f)
        {
            return static_cast<uint8_t>(f);
        }

        uint8_t mFlags = 0;
    };

    struct LapData {
        std::time_t timestamp = 0;      // UTC
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // seconds
        std::time_t elapsed   = 0;      // seconds
        float       distance  = 0.0f;   // m
        float       speedAvg  = 0.0f;   // m/s
        float       speedMax  = 0.0f;   // m/s
        float       hrAvg     = 0.0f;   // bpm
        float       hrMax     = 0.0f;   // bpm
        float       ascent    = 0.0f;   // m
        float       descent   = 0.0f;   // m
        FIT_MESSAGE_INDEX wktStepIndex = FIT_MESSAGE_INDEX_INVALID; // workout_step this lap belongs to (INVALID = none)
    };

    /**
     * @brief One step of a structured (interval) workout description.
     *
     * Encoded into a workout_step message. For a REPEAT step, durationValue is the
     * message_index of the first step to loop back to and repeatCount the number of
     * iterations; intensity is left unset.
     */
    struct WorkoutStepData {
        FIT_INTENSITY         intensity     = FIT_INTENSITY_INVALID;
        FIT_WKT_STEP_DURATION durationType  = FIT_WKT_STEP_DURATION_OPEN;
        FIT_UINT32            durationValue = 0;  // TIME: ms; DISTANCE: cm; OPEN: 0; REPEAT: first-step index
        FIT_UINT32            repeatCount   = 0;  // REPEAT only -> target_value (iterations)
    };

    struct TrackData {
        std::time_t timestamp = 0;      // UTC
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // seconds
        std::time_t elapsed   = 0;      // seconds
        float       distance  = 0.0f;   // m
        float       speedAvg  = 0.0f;   // m/s
        float       speedMax  = 0.0f;   // m/s
        float       hrAvg     = 0.0f;   // bpm
        float       hrMax     = 0.0f;   // bpm
        float       ascent    = 0.0f;   // m
        float       descent   = 0.0f;   // m
    };

    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void pause(std::time_t timestamp);
    void resume(std::time_t timestamp);
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    /// Emit the workout + workout_step messages describing a structured workout.
    void addWorkout(const char* name, const WorkoutStepData* steps, uint8_t count);
    void stop(const TrackData& track);
    void discard();

private:
    /// A constant reference to an Kernel object.
    const SDK::Kernel& mKernel;

    /// Path to FIT file
    const char* mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    uint16_t mLapCounter = 0;
    FIT_UINT16 mDataCRC = 0;

    SDK::Component::FitHelper mFHFileID;
    SDK::Component::FitHelper mFHDeveloper;
    SDK::Component::FitHelper mFHLap;
    SDK::Component::FitHelper mFHSession;
    SDK::Component::FitHelper mFHEvent;
    SDK::Component::FitHelper mFHActivity;
    SDK::Component::FitHelper mFHRecord;    // Record
    SDK::Component::FitHelper mFHRecordG;   // Record + GPS
    SDK::Component::FitHelper mFHRecordB;   // Record + Battery
    SDK::Component::FitHelper mFHRecordGB;  // Record + GPS + Battery

    SDK::Component::FitHelper mFHBatteryLevelField;
    SDK::Component::FitHelper mFHBatteryVoltageField;

    SDK::Component::FitHelper mFHWorkout;
    SDK::Component::FitHelper mFHWorkoutStep;

    enum class MsgNumber {
        FILE = 1,
        DEVELOP,
        RECORD,
        RECORD_G,
        RECORD_B,
        RECORD_GB,
        LAP,
        SESSION,
        ACTIVITY,
        EVENT,
        BATTERY,
        WORKOUT,
        WORKOUT_STEP
    };

    FIT_RECORD_MESG prepareRecordMsg(const RecordData& record);

    void AddMessageEvent(std::time_t t, FIT_EVENT_TYPE type);

    bool createAndOpenFile(std::time_t utc);
    void saveFile();
    void deleteFile();

    void saveSummary(const TrackData& track);

    static time_t tm2epoch(const struct tm* tm);
    static time_t epochToLocal(time_t utc);
    static FIT_DATE_TIME unixToFitTimestamp(std::time_t unixTimestamp);
    static FIT_SINT32 ConvertDegreesToSemicircles(float degrees);

    void WriteFileHeader(SDK::Interface::IFile* fp);
    void WriteCRC(SDK::Interface::IFile* fp);
};

#endif // ACTIVITY_WRITER_HPP
