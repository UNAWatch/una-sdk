/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @date    31-03-2026
 * @author  Denys Sobchuk <avatarsd2@gmail.com>
 * @brief   Serializes activity data to a FIT file.
 ******************************************************************************
 */

#ifndef __ACTIVITY_WRITER_HPP
#define __ACTIVITY_WRITER_HPP

#include <cstdint>
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
        std::time_t timestamp;  // UTC
        uint32_t appVersion;    // Application version 4 bytes LE [patch, minor, major, 0]
        std::string devID;      // Developer ID (max len 16)
        std::string appID;      // Application ID (max len 16)
    };

    struct RecordData {
        enum class Field : uint8_t {
            COORDS = 1u << 0,   // lat/long valid
            SPEED = 1u << 1,
            ALTITUDE = 1u << 2,
            HEART_RATE = 1u << 3,
            BATTERY = 1u << 4,
            STRAIN = 1u << 5,
            ACTIVE_MIN = 1u << 6,
        };

        void set(Field f) { mFlags |= mask(f); }
        void clear(Field f) { mFlags &= ~mask(f); }
        bool has(Field f) const { return (mFlags & mask(f)) != 0; }

        std::time_t timestamp;
        float latitude, longitude;
        float speed;  // m/s
        float altitude;  // m
        uint8_t heartRate;  // bpm
        uint8_t batteryLevel;  // %
        uint16_t batteryVoltage;  // mV
        float strain;
        uint32_t activeMin;

    private:
        static uint8_t mask(Field f) { return static_cast<uint8_t>(f); }
        uint8_t mFlags = 0;
    };

    struct LapData {
        std::time_t timestamp;  // UTC
        std::time_t timeStart;  // UTC
        std::time_t duration;   // seconds
        std::time_t elapsed;    // seconds
        uint8_t hrAvg;          // bpm
        uint8_t hrMax;          // bpm
        uint16_t totalAscent;   // m
        uint16_t totalDescent;  // m
    };

    struct TrackData {
        std::time_t timeStart;  // UTC
        std::time_t duration;   // seconds
        std::time_t elapsed;    // seconds
        uint8_t hrAvg;          // bpm
        uint8_t hrMax;          // bpm
        uint16_t totalAscent;   // m
        uint16_t totalDescent;  // m
    };


    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void pause();
    void resume();
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    void stop(const TrackData& track);
    void discard();
    
private:
    /// A constant reference to a Kernel object.
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
    SDK::Component::FitHelper mFHRecord;
    SDK::Component::FitHelper mFHStrainField;
    SDK::Component::FitHelper mFHActiveField;

    static constexpr uint8_t skFileMsgNum         = 1;
    static constexpr uint8_t skDevelopMsgNum      = 2;
    static constexpr uint8_t skRecordMsgNum       = 3;
    static constexpr uint8_t skLapMsgNum          = 4;
    static constexpr uint8_t skSessionMsgNum      = 5;
    static constexpr uint8_t skActivityMsgNum     = 6;
    static constexpr uint8_t skEventMsgNum        = 7;
    static constexpr uint8_t skStrainMsgNum       = 8;
    static constexpr uint8_t skActiveMsgNum       = 9;

    void AddMessageEvent(std::time_t t, FIT_EVENT_TYPE type);

    bool createAndOpenFile(std::time_t utc);
    void saveFile();
    void deleteFile();

    void testFitHelper(const AppInfo& info);

    static time_t tm2epoch(const struct tm* tm);
    static time_t epochToLocal(time_t utc);
    static FIT_DATE_TIME unixToFitTimestamp(std::time_t unixTimestamp);
    static FIT_SINT32 ConvertDegreesToSemicircles(float degrees);

    void WriteFileHeader(SDK::Interface::IFile* fp);
    void WriteCRC(SDK::Interface::IFile* fp);
};

#endif /* __ACTIVITY_WRITER_HPP */
