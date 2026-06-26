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
#include "SDK/Fit/FitWriter.hpp"

/**
 * @class ActivityWriter
 * @brief Serializes heart rate activity data to a FIT file.
 *
 * FIT file structure produced:
 *   FileHeader -> FileID -> DeveloperDataID -> FieldDescription(hr_trust_level)
 *   -> Event(START) -> Record* -> Lap -> Session -> Activity -> FileHeader(updated) -> FileCRC
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
        std::time_t timestamp  = 0;  // UTC
        uint8_t     heartRate  = 0;  // bpm
        uint8_t     trustLevel = 0;  // HR trust level
    };

    struct LapData {
        std::time_t timestamp = 0;  // UTC - lap end time
        std::time_t timeStart = 0;  // UTC - lap start time
        std::time_t duration  = 0;  // seconds
        std::time_t elapsed   = 0;  // seconds
        uint8_t     hrAvg     = 0;  // bpm
        uint8_t     hrMax     = 0;  // bpm
    };

    struct TrackData {
        std::time_t timestamp = 0;  // UTC - session end time
        std::time_t timeStart = 0;  // UTC - session start time
        std::time_t duration  = 0;  // seconds
        std::time_t elapsed   = 0;  // seconds
        uint8_t     hrAvg     = 0;  // bpm
        uint8_t     hrMax     = 0;  // bpm
    };


    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    void stop(const TrackData& track);
    void discard();

private:
    /// Local message types (FIT record header, 0-15).
    enum Local : uint8_t {
        L_FILE_ID = 0,
        L_DEV_ID,
        L_FIELD_DESC,
        L_EVENT,
        L_RECORD,
        L_LAP,
        L_SESSION,
        L_ACTIVITY,
    };

    /// Developer field definition numbers (UNA-assigned).
    enum DevField : uint8_t {
        DF_HR_TRUST_LEVEL = 0,
    };

    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    std::unique_ptr<SDK::Fit::FitWriter>   mFit  = nullptr;
    uint16_t mLapCounter = 0;

    void writeFieldDescription(uint8_t devFieldNum, const char* name,
                               const char* units, SDK::Fit::BaseType baseType);
    void addMessageEvent(std::time_t t, SDK::Fit::EventType type);

    bool createAndOpenFile(std::time_t utc);

    static std::time_t tm2epoch(const struct tm* tm);
    static std::time_t epochToLocal(std::time_t utc);
    static uint32_t    unixToFitTimestamp(std::time_t unixTimestamp);
};

#endif // ACTIVITY_WRITER_HPP
