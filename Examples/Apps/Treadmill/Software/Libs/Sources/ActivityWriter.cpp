/**
 ******************************************************************************
 * @file    ActivityWriter.cpp
 * @brief   Serializes activity data to a FIT file (native SDK::Fit encoder).
 ******************************************************************************
 */

#include "ActivityWriter.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cassert>
#include <cstring>

#define LOG_MODULE_PRX      "ActivityWriter"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace fit = SDK::Fit;
using Field = fit::FitWriter::Field;
using DevFieldDef = fit::FitWriter::DevField;

namespace {
    // Native record fields shared by every record variant (treadmill: no GPS,
    // no altitude -- cadence-driven speed, HR, cadence, model step length).
    // Field order here defines the on-wire order and must match the value-write
    // order in addRecord().
    const Field kRecordCommonTail[] = {
        fit::field::Record::EnhancedSpeed,
        fit::field::Record::HeartRate,
        fit::field::Record::Cadence,
        fit::field::Record::FractionalCadence,
        fit::field::Record::StepLength,
    };
}  // namespace

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

    // Developer field descriptions (label/units survive any profile).
    writeFieldDescription(DF_DISTANCE_PRECAL, "distance_pre_calibration", "m",
                          fit::BaseType::UInt32);  // session field (§5.4)
    writeFieldDescription(DF_BATTERY_LEVEL, "batteryLevel", "%", fit::BaseType::UInt8);
    writeFieldDescription(DF_BATTERY_VOLTAGE, "battVoltage", "mV", fit::BaseType::UInt16);
    writeFieldDescription(DF_HR_SOURCE, "hr_source", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(DF_HR_OPTICAL, "hr_optical", "bpm", fit::BaseType::UInt8);
    writeFieldDescription(DF_HR_EXTERNAL, "hr_external", "bpm", fit::BaseType::UInt8);

    // event
    mFit->defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});

    defineRecordMessages();

    // lap / session / activity. The session carries the pre-calibration distance
    // as a developer field (§5.4).
    mFit->defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
        {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
         fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
         fit::field::Lap::TotalDistance, fit::field::Lap::MessageIndex,
         fit::field::Lap::AvgSpeed, fit::field::Lap::MaxSpeed,
         fit::field::Lap::TotalAscent, fit::field::Lap::TotalDescent,
         fit::field::Lap::AvgHeartRate, fit::field::Lap::MaxHeartRate,
         fit::field::Lap::WktStepIndex});
    mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
        {fit::field::Session::Timestamp, fit::field::Session::StartTime,
         fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
         fit::field::Session::TotalDistance, fit::field::Session::MessageIndex,
         fit::field::Session::AvgSpeed, fit::field::Session::MaxSpeed,
         fit::field::Session::TotalAscent, fit::field::Session::TotalDescent,
         fit::field::Session::NumLaps, fit::field::Session::Sport,
         fit::field::Session::SubSport, fit::field::Session::AvgHeartRate,
         fit::field::Session::MaxHeartRate},
        {{DF_DISTANCE_PRECAL, 4, 0}});
    mFit->defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
        {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
         fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});

    addMessageEvent(info.timestamp, fit::EventType::Start);

    // Header + all definitions are on disk: flush and drop the recovery marker.
    // getPosition() here is a clean record boundary, so a crash after this point
    // is recoverable up to at least the header/definitions. Only write the
    // marker when begin() succeeded: a failed begin() leaves a near-empty/broken
    // .fit that the recovery marker must not point next boot's recover() at.
    if (begun) {
        mFile->flush();
        mMarker.write(mFile->getPath(), static_cast<uint32_t>(mFile->getPosition()));
    }
}

void ActivityWriter::defineRecordMessages()
{
    const DevFieldDef hr3[] = {
        {DF_HR_SOURCE, 1, 0}, {DF_HR_OPTICAL, 1, 0}, {DF_HR_EXTERNAL, 1, 0},
    };
    const DevFieldDef batt5[] = {
        {DF_BATTERY_LEVEL, 1, 0}, {DF_BATTERY_VOLTAGE, 2, 0},
        {DF_HR_SOURCE, 1, 0}, {DF_HR_OPTICAL, 1, 0}, {DF_HR_EXTERNAL, 1, 0},
    };

    // Plain record (speed/HR/cadence only) + 3 HR developer fields.
    mFit->defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp,
         kRecordCommonTail[0], kRecordCommonTail[1], kRecordCommonTail[2],
         kRecordCommonTail[3], kRecordCommonTail[4]},
        {hr3[0], hr3[1], hr3[2]});

    // + battery (5 developer fields).
    mFit->defineMessage(L_RECORD_B, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp,
         kRecordCommonTail[0], kRecordCommonTail[1], kRecordCommonTail[2],
         kRecordCommonTail[3], kRecordCommonTail[4]},
        {batt5[0], batt5[1], batt5[2], batt5[3], batt5[4]});
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

void ActivityWriter::pause(std::time_t timestamp)
{
    if (mFit) {
        addMessageEvent(timestamp, fit::EventType::Stop);
    }
}

void ActivityWriter::resume(std::time_t timestamp)
{
    if (mFit) {
        addMessageEvent(timestamp, fit::EventType::Start);
    }
}

void ActivityWriter::addRecord(const RecordData& record)
{
    if (!mFit) {
        return;
    }

    const bool batt = record.has(RecordData::Field::BATTERY);
    const uint8_t local = batt ? L_RECORD_B : L_RECORD;

    fit::FitWriter::Data d = mFit->data(local);

    d.u32(unixToFitTimestamp(record.timestamp));
    // enhanced_speed (1000 * m/s)
    d.u32(record.has(RecordData::Field::SPEED)
              ? static_cast<uint32_t>(record.speed * 1000.0f)
              : static_cast<uint32_t>(fit::baseTypeInvalid(fit::BaseType::UInt32)));
    d.u8(record.has(RecordData::Field::HEART_RATE)
             ? static_cast<uint8_t>(record.heartRate)
             : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));

    if (record.has(RecordData::Field::CADENCE)) {
        const auto c = SDK::FitRecordCadence::encodeCadenceSpm(record.cadenceSpm);
        d.u8(c.cadence).u8(c.fractionalCadence);
    } else {
        d.u8(static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));
        d.u8(static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));
    }

    d.u16(record.has(RecordData::Field::STEP_LENGTH)
              ? SDK::FitRecordCadence::encodeStepLengthM(record.stepLengthM)
              : static_cast<uint16_t>(fit::baseTypeInvalid(fit::BaseType::UInt16)));

    // Developer fields, in definition order.
    if (batt) {
        d.u8(record.batteryLevel).u16(record.batteryVoltage);
    }
    d.u8(record.hrSource).u8(record.hrOpticalBpm).u8(record.hrExternalBpm);

    d.write();

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
        .u32(static_cast<uint32_t>(lap.distance * 100))
        .u16(0)  // message_index
        .u16(static_cast<uint16_t>(lap.speedAvg * 1000))
        .u16(static_cast<uint16_t>(lap.speedMax * 1000))
        .u16(static_cast<uint16_t>(lap.ascent))
        .u16(static_cast<uint16_t>(lap.descent))
        .u8(static_cast<uint8_t>(lap.hrAvg))
        .u8(static_cast<uint8_t>(lap.hrMax))
        .u16(lap.wktStepIndex)
        .write();
    mLapCounter++;

    // Laps are sparse: flush and advance the marker, but only when the flush
    // durably landed (else keep the previous good offset).
    if (mFile->flush()) {
        mMarker.update(static_cast<uint32_t>(mFile->getPosition()));
        mLastFlushUtc = lap.timestamp;
    }
}

void ActivityWriter::addWorkout(const char* name, const WorkoutStepData* steps, uint8_t count)
{
    if (!mFit || steps == nullptr || count == 0) {
        return;
    }

    const uint8_t nameLen = name ? static_cast<uint8_t>(std::strlen(name) + 1) : 1;
    mFit->defineMessage(L_WORKOUT, fit::mesgNum(fit::MesgNum::Workout),
        {fit::field::Workout::MessageIndex,
         {fit::field::Workout::kWktNameNum, fit::BaseType::String, nameLen},
         fit::field::Workout::NumValidSteps, fit::field::Workout::Sport});
    mFit->defineMessage(L_WORKOUT_STEP, fit::mesgNum(fit::MesgNum::WorkoutStep),
        {fit::field::WorkoutStep::MessageIndex, fit::field::WorkoutStep::DurationType,
         fit::field::WorkoutStep::DurationValue, fit::field::WorkoutStep::TargetType,
         fit::field::WorkoutStep::TargetValue, fit::field::WorkoutStep::Intensity});

    mFit->data(L_WORKOUT)
        .u16(0)
        .str(name ? name : "", nameLen)
        .u16(count)
        .u8(static_cast<uint8_t>(fit::Sport::Running))
        .write();

    for (uint8_t i = 0; i < count; ++i) {
        const bool repeat = steps[i].durationType == fit::WktStepDuration::RepeatUntilStepsComplete;
        mFit->data(L_WORKOUT_STEP)
            .u16(i)
            .u8(static_cast<uint8_t>(steps[i].durationType))
            .u32(steps[i].durationValue)
            .u8(repeat ? static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::Enum))
                       : static_cast<uint8_t>(fit::WktStepTarget::Open))
            .u32(repeat ? steps[i].repeatCount : 0u)
            .u8(repeat ? static_cast<uint8_t>(fit::Intensity::Invalid)
                       : static_cast<uint8_t>(steps[i].intensity))
            .write();
    }
}

bool ActivityWriter::stop(const TrackData& track)
{
    if (!mFit) {
        return false;
    }

    bool ok = mFit->ok();

    ok = mFit->data(L_SESSION)
        .u32(unixToFitTimestamp(track.timestamp))
        .u32(unixToFitTimestamp(track.timeStart))
        .u32(static_cast<uint32_t>(track.elapsed * 1000))
        .u32(static_cast<uint32_t>(track.duration * 1000))
        .u32(static_cast<uint32_t>(track.distance * 100))
        .u16(0)  // message_index
        .u16(static_cast<uint16_t>(track.speedAvg * 1000))
        .u16(static_cast<uint16_t>(track.speedMax * 1000))
        .u16(static_cast<uint16_t>(track.ascent))
        .u16(static_cast<uint16_t>(track.descent))
        .u16(mLapCounter)
        .u8(static_cast<uint8_t>(fit::Sport::Running))
        .u8(static_cast<uint8_t>(fit::SubSport::Treadmill))  // indoor treadmill run (§7.3)
        .u8(static_cast<uint8_t>(track.hrAvg))
        .u8(static_cast<uint8_t>(track.hrMax))
        // Developer field: estimated distance before Calibrate & Save (100 * m).
        .u32(static_cast<uint32_t>(track.distancePreCalibrationM * 100.0f))
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
    } else {
        ok = false;
    }

    // The .fit is durably finished on disk: drop the recovery marker so the
    // next boot does not treat this activity as interrupted.
    if (ok) {
        mMarker.remove();
    }

    const bool summaryOk = saveSummary(track);
    return ok && summaryOk;
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

    int len = snprintf(buff, sizeof(buff), "%s/%04u%02u/", mPath,
                       localTime.tm_year + 1900, localTime.tm_mon + 1);
    if (len <= 0 || !mKernel.fs.mkdir(buff)) {
        LOG_ERROR("Failed to create dir [%s]\n", buff);
        return false;
    }

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

bool ActivityWriter::saveSummary(const TrackData& track)
{
    if (!mFile) {
        return false;
    }
    char buff[256]{};
    size_t nameLen = strlen(mFile->getPath());
    snprintf(buff, sizeof(buff), "%.*s%s", static_cast<int>(nameLen - 3), mFile->getPath(), "json");

    mFile->setPath(buff);

    if (!mFile->open(true, true)) {
        LOG_ERROR("Failed to open activity summary [%s]\n", buff);
        mFile.reset();
        return false;
    }

    SDK::JsonStreamWriter writer(mFile.get());
    writer.startMap();
    writer.add("time_start", static_cast<uint32_t>(track.timeStart));
    writer.add("duration", static_cast<uint32_t>(track.duration));
    writer.add("distance", track.distance);
    writer.add("hr_avg", track.hrAvg);
    writer.add("elevation", track.ascent - track.descent);
    writer.add("activity_type", "treadmill");
    writer.endMap();

    const bool ok = mFile->flush();
    return mFile->close() && ok;
}

std::time_t ActivityWriter::tm2epoch(const struct tm* tm)
{
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;
    if (m <= 2) { y -= 1; m += 12; }
    int64_t  era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + (int64_t)doe - 719468;
    int64_t secs = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
    return (std::time_t)secs;
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
    const std::time_t FIT_EPOCH_OFFSET = 631065600;
    return static_cast<uint32_t>(unixTimestamp - FIT_EPOCH_OFFSET);
}
