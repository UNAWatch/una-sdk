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
#include <memory>
#include <string>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/FitHelper/FitHelper.hpp"

extern "C" {
#include "fit_example.h"
}

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
    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    uint16_t   mLapCounter = 0;
    FIT_UINT16 mDataCRC    = 0;

    SDK::Component::FitHelper mFHFileID;
    SDK::Component::FitHelper mFHDeveloper;
    SDK::Component::FitHelper mFHLap;
    SDK::Component::FitHelper mFHSession;
    SDK::Component::FitHelper mFHEvent;
    SDK::Component::FitHelper mFHActivity;
    SDK::Component::FitHelper mFHRecord;
    SDK::Component::FitHelper mFHTrustLevelField;

    enum class MsgNumber : uint8_t {
        FILE         = 1,
        DEVELOP,
        RECORD,
        LAP,
        SESSION,
        ACTIVITY,
        EVENT,
        HR_TRUST_LEVEL,
    };

    void addMessageEvent(std::time_t t, FIT_EVENT_TYPE type);

    bool createAndOpenFile(std::time_t utc);
    void saveFile();
    void deleteFile();

    static time_t        tm2epoch(const struct tm* tm);
    static time_t        epochToLocal(time_t utc);
    static FIT_DATE_TIME unixToFitTimestamp(std::time_t unixTimestamp);

    void writeFileHeader(SDK::Interface::IFile* fp);
    void writeCRC(SDK::Interface::IFile* fp);
};

#endif // ACTIVITY_WRITER_HPP
