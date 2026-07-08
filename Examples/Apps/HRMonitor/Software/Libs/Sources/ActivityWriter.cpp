/**
 ******************************************************************************
 * @file    ActivityWriter.cpp
 * @brief   Serializes activity data to a FIT file (native SDK::Fit encoder).
 ******************************************************************************
 */

#include "ActivityWriter.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"

#include <cassert>
#include <cstring>

#define LOG_MODULE_PRX      "ActivityWriter"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace fit = SDK::Fit;
using DevFieldDef = fit::FitWriter::DevField;

ActivityWriter::ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir)
    : mKernel(kernel), mPath(pathToDir), mMarker(kernel.fs, pathToDir)
{
    assert(pathToDir != nullptr);
}

void ActivityWriter::start(const AppInfo& info)
{
    mLapCounter   = 0;
    mLastFlushUtc = info.timestamp;

    if (!createAndOpenFile(info.timestamp)) {
        return;
    }

    mFit = std::make_unique<fit::FitWriter>(*mFile);
    const bool begun = mFit->begin(/*profileVersion=*/0);
    if (!begun) {
        LOG_ERROR("Failed to write FIT header\n");
    }

    // file_id
    mFit->defineMessage(L_FILE_ID, fit::mesgNum(fit::MesgNum::FileId),
        {fit::field::FileId::Type, fit::field::FileId::Manufacturer,
         fit::field::FileId::Product, fit::field::FileId::SerialNumber,
         fit::field::FileId::TimeCreated});
    mFit->data(L_FILE_ID)
        .u8(static_cast<uint8_t>(fit::File::Activity))
        .u16(static_cast<uint16_t>(fit::Manufacturer::Development))
        .u16(0)
        .u32(0)
        .u32(unixToFitTimestamp(info.timestamp))
        .write();

    // developer_data_id
    mFit->defineMessage(L_DEV_ID, fit::mesgNum(fit::MesgNum::DeveloperDataId),
        {fit::field::DeveloperDataId::ApplicationId,
         fit::field::DeveloperDataId::DeveloperDataIndex});
    {
        uint8_t appId[16] = {};
        std::strncpy(reinterpret_cast<char*>(appId), info.appID.c_str(), sizeof(appId));
        mFit->data(L_DEV_ID).bytes(appId, sizeof(appId)).u8(0).write();
    }

    // Developer field: hr_trust_level (label/units survive any profile).
    writeFieldDescription(DF_HR_TRUST_LEVEL, "hr_trust_level", "percents", fit::BaseType::UInt8);

    // event
    mFit->defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});

    // record (HR only) + hr_trust_level developer field
    {
        const DevFieldDef hrTrust[] = {{DF_HR_TRUST_LEVEL, 1, 0}};
        mFit->defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
            {fit::field::Record::Timestamp, fit::field::Record::HeartRate},
            {hrTrust[0]});
    }

    // lap / session / activity
    mFit->defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
        {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
         fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
         fit::field::Lap::MessageIndex, fit::field::Lap::AvgHeartRate,
         fit::field::Lap::MaxHeartRate});
    mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
        {fit::field::Session::Timestamp, fit::field::Session::StartTime,
         fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
         fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
         fit::field::Session::Sport, fit::field::Session::SubSport,
         fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate});
    mFit->defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
        {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
         fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});

    addMessageEvent(info.timestamp, fit::EventType::Start);

    // Header + all definitions are on disk: flush and drop the recovery marker.
    // getPosition() here is a clean record boundary, so a crash after this point
    // is recoverable up to at least the header/definitions. Only write the
    // marker when begin() succeeded AND the initial flush is durable: a failed
    // begin()/flush leaves a near-empty/broken or non-durable .fit that the
    // recovery marker must not point next boot's recover() at.
    if (begun && mFile->flush()) {
        mMarker.write(mFile->getPath(), static_cast<uint32_t>(mFile->getPosition()));
    }
}

void ActivityWriter::writeFieldDescription(uint8_t devFieldNum, const char* name,
                                           const char* units, fit::BaseType baseType)
{
    const uint8_t nameLen  = name ? static_cast<uint8_t>(std::strlen(name) + 1) : 1;
    const uint8_t unitsLen = units ? static_cast<uint8_t>(std::strlen(units) + 1) : 1;

    // Redefine the field_description slot to size the name/units strings exactly.
    mFit->defineMessage(L_FIELD_DESC, fit::mesgNum(fit::MesgNum::FieldDescription),
        {fit::field::FieldDescription::DeveloperDataIndex,
         fit::field::FieldDescription::FieldDefinitionNumber,
         fit::field::FieldDescription::FitBaseTypeId,
         {fit::field::FieldDescription::kFieldNameNum, fit::BaseType::String, nameLen},
         {fit::field::FieldDescription::kUnitsNum, fit::BaseType::String, unitsLen}});
    mFit->data(L_FIELD_DESC)
        .u8(0)
        .u8(devFieldNum)
        .u8(fit::baseTypeId(baseType))
        .str(name ? name : "", nameLen)
        .str(units ? units : "", unitsLen)
        .write();
}

void ActivityWriter::addRecord(const RecordData& record)
{
    if (!mFit) {
        return;
    }

    mFit->data(L_RECORD)
        .u32(unixToFitTimestamp(record.timestamp))
        .u8(record.heartRate)
        .u8(record.trustLevel)  // developer field hr_trust_level
        .write();

    // Periodic durability flush: sync to eMMC and advance the marker to this
    // record boundary so a later crash recovers a record-complete file.
    if (record.timestamp - mLastFlushUtc >= skFlushIntervalSec) {
        // Only advance the marker when the flush durably landed; otherwise keep
        // the previous good offset (never point recover() past non-durable data).
        if (mFile->flush()) {
            mMarker.update(static_cast<uint32_t>(mFile->getPosition()));
            mLastFlushUtc = record.timestamp;
        }
    }
}

void ActivityWriter::addLap(const LapData& lap)
{
    if (!mFit) {
        return;
    }
    mFit->data(L_LAP)
        .u32(unixToFitTimestamp(lap.timestamp))
        .u32(unixToFitTimestamp(lap.timeStart))
        .u32(static_cast<uint32_t>(lap.elapsed * 1000))
        .u32(static_cast<uint32_t>(lap.duration * 1000))
        .u16(0)  // message_index
        .u8(lap.hrAvg)
        .u8(lap.hrMax)
        .write();
    mLapCounter++;

    // Laps are sparse: flush and advance the marker, but only when the flush
    // durably landed (else keep the previous good offset).
    if (mFile->flush()) {
        mMarker.update(static_cast<uint32_t>(mFile->getPosition()));
        mLastFlushUtc = lap.timestamp;
    }
}

bool ActivityWriter::stop(const TrackData& track)
{
    if (!mFit) {
        return false;
    }

    // STOP event
    addMessageEvent(track.timestamp, fit::EventType::Stop);

    bool ok = mFit->ok();

    ok = mFit->data(L_SESSION)
        .u32(unixToFitTimestamp(track.timestamp))
        .u32(unixToFitTimestamp(track.timeStart))
        .u32(static_cast<uint32_t>(track.elapsed * 1000))
        .u32(static_cast<uint32_t>(track.duration * 1000))
        .u16(0)  // message_index
        .u16(mLapCounter)
        .u8(static_cast<uint8_t>(fit::Sport::Generic))
        .u8(static_cast<uint8_t>(fit::SubSport::Generic))
        .u8(track.hrAvg)
        .u8(track.hrMax)
        .write() && ok;

    ok = mFit->data(L_ACTIVITY)
        .u32(unixToFitTimestamp(track.timestamp))
        .u32(static_cast<uint32_t>(track.duration * 1000))
        .u32(unixToFitTimestamp(epochToLocal(track.timestamp)))
        .u16(1)
        .write() && ok;

    const bool finishOk = mFit->finish();
    if (!finishOk) {
        LOG_ERROR("Failed to finalize FIT file\n");
    }
    ok = finishOk && mFit->ok() && ok;
    mFit.reset();

    if (mFile) {
        ok = mFile->flush() && ok;
        ok = mFile->close() && ok;
        mFile.reset();
    } else {
        ok = false;
    }

    // The .fit is durably finished on disk: drop the recovery marker so the
    // next boot does not treat this activity as interrupted.
    if (ok) {
        mMarker.remove();
    }

    return ok;
}

void ActivityWriter::discard()
{
    mFit.reset();
    mMarker.remove();
    if (!mFile) {
        return;
    }
    if (mFile->isOpen()) {
        mFile->close();
    }
    mFile->remove();
    mFile.reset();
}

void ActivityWriter::addMessageEvent(std::time_t t, fit::EventType type)
{
    mFit->data(L_EVENT)
        .u32(unixToFitTimestamp(t))
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(type))
        .write();
}

bool ActivityWriter::recoverInterrupted()
{
    // All marker I/O + FitWriter::recover() orchestration lives in the shared
    // SDK::Fit::RecordingMarker. Recovery needs no sibling .json to register a
    // recovered activity (the kernel's activity registry tracks .fit files
    // only), so this is pure wiring.
    const auto result = mMarker.recover();
    if (result.recovered) {
        LOG_INFO("Recovery: finalized interrupted activity [%s]\n", result.path.c_str());
    }
    return result.recovered;
}

bool ActivityWriter::createAndOpenFile(std::time_t utc)
{
    char buff[256]{};
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif

    // Create directory
    int len = snprintf(buff, sizeof(buff), "%s/%04u%02u/", mPath,
                       localTime.tm_year + 1900, localTime.tm_mon + 1);
    if (len <= 0 || !mKernel.fs.mkdir(buff)) {
        LOG_ERROR("Failed to create dir [%s]\n", buff);
        return false;
    }

    // Create file
    snprintf(&buff[len], sizeof(buff) - len, "activity_%04u%02u%02uT%02u%02u%02u.fit",
        localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
        localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

    mFile = mKernel.fs.file(buff);
    if (!mFile || !mFile->open(true, true)) {
        LOG_ERROR("Failed to create file [%s]\n", buff);
        mFile.reset();
        return false;
    }

    return true;
}

std::time_t ActivityWriter::tm2epoch(const struct tm* tm)
{
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    int64_t  era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = static_cast<uint32_t>(y - era * 400);
    uint32_t doy = (153 * (m - 3) + 2) / 5 + tm->tm_mday - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t  days = era * 146097 + static_cast<int64_t>(doe) - 719468;
    int64_t  secs = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;

    return static_cast<std::time_t>(secs);
}

std::time_t ActivityWriter::epochToLocal(std::time_t utc)
{
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif
    return tm2epoch(&localTime);
}

uint32_t ActivityWriter::unixToFitTimestamp(std::time_t unixTimestamp)
{
    static constexpr std::time_t skFitEpochOffset = 631065600;  // seconds between 1970-01-01 and 1989-12-31
    return static_cast<uint32_t>(unixTimestamp - skFitEpochOffset);
}
