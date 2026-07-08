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
#include "SDK/Fit/RecordingMarker.hpp"

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
    /// Finalize the current activity. Returns true only if the FIT stream and
    /// its finish()/flush/close all succeed.
    bool stop(const TrackData& track);
    void discard();

    /// Finalize an activity that a previous boot left unfinished (power loss /
    /// crash mid-recording). If the recovery marker exists it names the torn
    /// .fit and the last record-complete data-end offset; the file is finalized
    /// via SDK::Fit::FitWriter::recover() and the marker is removed. Returns true
    /// only when an interrupted activity was recovered into a valid FIT file.
    /// Safe (returns false, no side effects) when no marker is present. Must run
    /// before any new activity is started.
    bool recoverInterrupted();

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

    /// Flush + marker-refresh cadence during recording (seconds of record time).
    static constexpr std::time_t skFlushIntervalSec = 30;

    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    std::unique_ptr<SDK::Fit::FitWriter>   mFit  = nullptr;
    SDK::Fit::RecordingMarker              mMarker;   ///< Shared crash-recovery marker.
    uint16_t    mLapCounter   = 0;
    std::time_t mLastFlushUtc = 0;   ///< Record timestamp of the last durability flush.

    void writeFieldDescription(uint8_t devFieldNum, const char* name,
                               const char* units, SDK::Fit::BaseType baseType);
    void addMessageEvent(std::time_t t, SDK::Fit::EventType type);

    bool createAndOpenFile(std::time_t utc);

    static std::time_t tm2epoch(const struct tm* tm);
    static std::time_t epochToLocal(std::time_t utc);
    static uint32_t    unixToFitTimestamp(std::time_t unixTimestamp);
};

#endif // ACTIVITY_WRITER_HPP
