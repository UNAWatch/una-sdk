/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @brief   Serializes activity data to a FIT file (native SDK::Fit encoder).
 ******************************************************************************
 */

#ifndef ACTIVITY_WRITER_HPP
#define ACTIVITY_WRITER_HPP

#include <cstdint>
#include <ctime>
#include <memory>
#include <string>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitRecordCadence.hpp"
#include "SDK/Fit/FitWriter.hpp"

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
            HEART_RATE = 1u << 3,
            BATTERY    = 1u << 4,
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
        float       heartRate      = 0.0f;  // bpm (arbitrated)
        uint8_t     hrSource       = 0;     // HeartRateEx::Source (0=none,1=optical,2=external)
        uint8_t     hrOpticalBpm   = 0;     // raw wrist-optical (PPG) bpm (0 = none)
        uint8_t     hrExternalBpm  = 0;     // raw external strap bpm (0 = none)
        uint8_t     batteryLevel   = 0;     // %
        uint16_t    batteryVoltage = 0;     // mV

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
        uint32_t    steps     = 0;      // count
        uint32_t    floors    = 0;      // count
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
        uint32_t    steps     = 0;      // count
        uint32_t    floors    = 0;      // count
    };

    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void pause(std::time_t timestamp);
    void resume(std::time_t timestamp);
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    void stop(const TrackData& track);
    void discard();

private:
    /// Local message types (FIT record header, 0-15).
    enum Local : uint8_t {
        L_FILE_ID = 0,
        L_DEV_ID,
        L_FIELD_DESC,   // reused for each field_description (redefined per string size)
        L_EVENT,
        L_RECORD,       // no GPS, no battery
        L_RECORD_G,     // + GPS
        L_RECORD_B,     // + battery
        L_RECORD_GB,    // + GPS + battery
        L_LAP,
        L_SESSION,
        L_ACTIVITY,
    };

    /// Developer field definition numbers (UNA-assigned).
    enum DevField : uint8_t {
        DF_STEPS           = 0,
        DF_FLOORS          = 1,
        DF_BATTERY_LEVEL   = 2,
        DF_BATTERY_VOLTAGE = 3,
        DF_HR_SOURCE       = 4,
        DF_HR_OPTICAL      = 5,
        DF_HR_EXTERNAL     = 6,
    };

    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    std::unique_ptr<SDK::Fit::FitWriter>   mFit  = nullptr;
    uint16_t mLapCounter = 0;

    void defineRecordMessages();
    void writeFieldDescription(uint8_t devFieldNum, const char* name,
                               const char* units, SDK::Fit::BaseType baseType);
    void addMessageEvent(std::time_t t, SDK::Fit::EventType type);

    bool createAndOpenFile(std::time_t utc);
    void saveSummary(const TrackData& track);

    static std::time_t tm2epoch(const struct tm* tm);
    static std::time_t epochToLocal(std::time_t utc);
    static uint32_t unixToFitTimestamp(std::time_t unixTimestamp);
    static int32_t  degreesToSemicircles(float degrees);
};

#endif // ACTIVITY_WRITER_HPP
