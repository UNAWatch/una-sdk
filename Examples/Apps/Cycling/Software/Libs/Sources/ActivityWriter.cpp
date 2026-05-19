/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes activity data to a file.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "ActivityWriter.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

extern "C" {
#include "fit_product.h"
#include "fit_crc.h"
}

#include <cassert>
#include <cstring>

#define LOG_MODULE_PRX      "ActivityWriter"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

ActivityWriter::ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir)
    : mKernel(kernel), mPath(pathToDir)
    , mFHFileID(static_cast<uint8_t>(MsgNumber::FILE), fit_mesg_defs[FIT_MESG_FILE_ID])
    , mFHDeveloper(static_cast<uint8_t>(MsgNumber::DEVELOP), fit_mesg_defs[FIT_MESG_DEVELOPER_DATA_ID])
    , mFHLap(static_cast<uint8_t>(MsgNumber::LAP), fit_mesg_defs[FIT_MESG_LAP])
    , mFHSession(static_cast<uint8_t>(MsgNumber::SESSION), fit_mesg_defs[FIT_MESG_SESSION])
    , mFHEvent(static_cast<uint8_t>(MsgNumber::EVENT), fit_mesg_defs[FIT_MESG_EVENT])
    , mFHActivity(static_cast<uint8_t>(MsgNumber::ACTIVITY), fit_mesg_defs[FIT_MESG_ACTIVITY])
    , mFHRecord(static_cast<uint8_t>(MsgNumber::RECORD), fit_mesg_defs[FIT_MESG_RECORD])
    , mFHRecordG(static_cast<uint8_t>(MsgNumber::RECORD_G), fit_mesg_defs[FIT_MESG_RECORD])
    , mFHRecordB(static_cast<uint8_t>(MsgNumber::RECORD_B), fit_mesg_defs[FIT_MESG_RECORD])
    , mFHRecordGB(static_cast<uint8_t>(MsgNumber::RECORD_GB), fit_mesg_defs[FIT_MESG_RECORD])
    , mFHBatteryLevelField(static_cast<uint8_t>(MsgNumber::BATTERY), 2, { &mFHRecordB, &mFHRecordGB })
    , mFHBatteryVoltageField(static_cast<uint8_t>(MsgNumber::BATTERY), 3, { &mFHRecordB, &mFHRecordGB })
{
    assert(pathToDir != nullptr);

    mFHFileID.init();

    mFHDeveloper.init();

    mFHLap.init({ FIT_LAP_FIELD_NUM_TIMESTAMP,
                  FIT_LAP_FIELD_NUM_START_TIME,
                  FIT_LAP_FIELD_NUM_TOTAL_ELAPSED_TIME,
                  FIT_LAP_FIELD_NUM_TOTAL_TIMER_TIME,
                  FIT_LAP_FIELD_NUM_TOTAL_DISTANCE,
                  FIT_LAP_FIELD_NUM_MESSAGE_INDEX,
                  FIT_LAP_FIELD_NUM_AVG_SPEED,
                  FIT_LAP_FIELD_NUM_MAX_SPEED,
                  FIT_LAP_FIELD_NUM_TOTAL_ASCENT,
                  FIT_LAP_FIELD_NUM_TOTAL_DESCENT,
                  FIT_LAP_FIELD_NUM_AVG_HEART_RATE,
                  FIT_LAP_FIELD_NUM_MAX_HEART_RATE });

    mFHSession.init({ FIT_SESSION_FIELD_NUM_TIMESTAMP,
                      FIT_SESSION_FIELD_NUM_START_TIME,
                      FIT_SESSION_FIELD_NUM_TOTAL_ELAPSED_TIME,
                      FIT_SESSION_FIELD_NUM_TOTAL_TIMER_TIME,
                      FIT_SESSION_FIELD_NUM_TOTAL_DISTANCE,
                      FIT_SESSION_FIELD_NUM_MESSAGE_INDEX,
                      FIT_SESSION_FIELD_NUM_AVG_SPEED,
                      FIT_SESSION_FIELD_NUM_MAX_SPEED,
                      FIT_SESSION_FIELD_NUM_TOTAL_ASCENT,
                      FIT_SESSION_FIELD_NUM_TOTAL_DESCENT,
                      FIT_SESSION_FIELD_NUM_NUM_LAPS,
                      FIT_SESSION_FIELD_NUM_SPORT,
                      FIT_SESSION_FIELD_NUM_SUB_SPORT,
                      FIT_SESSION_FIELD_NUM_AVG_HEART_RATE,
                      FIT_SESSION_FIELD_NUM_MAX_HEART_RATE });

    mFHEvent.init({ FIT_EVENT_FIELD_NUM_TIMESTAMP,
                    FIT_EVENT_FIELD_NUM_EVENT,
                    FIT_EVENT_FIELD_NUM_EVENT_TYPE });

    mFHActivity.init({ FIT_ACTIVITY_FIELD_NUM_TIMESTAMP,
                       FIT_ACTIVITY_FIELD_NUM_TOTAL_TIMER_TIME,
                       FIT_ACTIVITY_FIELD_NUM_LOCAL_TIMESTAMP,
                       FIT_ACTIVITY_FIELD_NUM_NUM_SESSIONS });

    mFHRecord.init({ FIT_RECORD_FIELD_NUM_TIMESTAMP,
                     FIT_RECORD_FIELD_NUM_ENHANCED_ALTITUDE,
                     FIT_RECORD_FIELD_NUM_ENHANCED_SPEED,
                     FIT_RECORD_FIELD_NUM_HEART_RATE });

    mFHRecordG.init({ FIT_RECORD_FIELD_NUM_TIMESTAMP,
                      FIT_RECORD_FIELD_NUM_POSITION_LAT,
                      FIT_RECORD_FIELD_NUM_POSITION_LONG,
                      FIT_RECORD_FIELD_NUM_ENHANCED_ALTITUDE,
                      FIT_RECORD_FIELD_NUM_ENHANCED_SPEED,
                      FIT_RECORD_FIELD_NUM_HEART_RATE });

    mFHRecordB.init({ FIT_RECORD_FIELD_NUM_TIMESTAMP,
                      FIT_RECORD_FIELD_NUM_ENHANCED_ALTITUDE,
                      FIT_RECORD_FIELD_NUM_ENHANCED_SPEED,
                      FIT_RECORD_FIELD_NUM_HEART_RATE });

    mFHRecordGB.init({ FIT_RECORD_FIELD_NUM_TIMESTAMP,
                       FIT_RECORD_FIELD_NUM_POSITION_LAT,
                       FIT_RECORD_FIELD_NUM_POSITION_LONG,
                       FIT_RECORD_FIELD_NUM_ENHANCED_ALTITUDE,
                       FIT_RECORD_FIELD_NUM_ENHANCED_SPEED,
                       FIT_RECORD_FIELD_NUM_HEART_RATE });

    mFHBatteryLevelField.init({ FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME,
                                FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS,
                                FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,
                                FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER,
                                FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID });

    mFHBatteryVoltageField.init({ FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME,
                                  FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS,
                                  FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,
                                  FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER,
                                  FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID });
}

void ActivityWriter::start(const AppInfo& info)
{
    // Reset counter
    mLapCounter = 0;
    mDataCRC = 0;

    createAndOpenFile(info.timestamp);

    if (!mFile) {
        return;
    }

    SDK::Interface::IFile* fp = mFile.get();

    // Add empty header
    WriteFileHeader(fp);

    // Write file id message.
    {
        mFHFileID.writeDef(fp);

        FIT_FILE_ID_MESG file_id_mesg{};
        strncpy(file_id_mesg.product_name, "UNA Watch", FIT_FILE_ID_MESG_PRODUCT_NAME_COUNT);
        file_id_mesg.serial_number = 0;
        file_id_mesg.time_created = unixToFitTimestamp(info.timestamp);
        file_id_mesg.manufacturer = FIT_MANUFACTURER_DEVELOPMENT;
        file_id_mesg.product = 0;
        file_id_mesg.number = 0;
        file_id_mesg.type = FIT_FILE_ACTIVITY;

        mFHFileID.writeMessage(&file_id_mesg, fp);
    }

    // Developer Data ID Message 
    {
        mFHDeveloper.writeDef(fp);

        FIT_DEVELOPER_DATA_ID_MESG developer{};
        strncpy(reinterpret_cast<char*>(developer.developer_id), info.devID.c_str(), FIT_DEVELOPER_DATA_ID_MESG_DEVELOPER_ID_COUNT);
        strncpy(reinterpret_cast<char*>(developer.application_id), info.appID.c_str(), FIT_DEVELOPER_DATA_ID_MESG_APPLICATION_ID_COUNT);
        developer.application_version  = info.appVersion;
        developer.manufacturer_id      = FIT_MANUFACTURER_DEVELOPMENT;
        developer.developer_data_index = 0;

        mFHDeveloper.writeMessage(&developer, fp);
    }

    // Additional fields
    {
        // Field 0: "battery level in percents"
        mFHBatteryLevelField.writeDef(fp);
        FIT_FIELD_DESCRIPTION_MESG battLevel{};
        strncpy(battLevel.field_name, "batteryLevel", FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT - 1);
        strncpy(battLevel.units, "%", FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT - 1);
        battLevel.developer_data_index    = 0;
        battLevel.field_definition_number = mFHBatteryLevelField.getFieldID();
        battLevel.fit_base_type_id        = FIT_BASE_TYPE_UINT8;
        mFHBatteryLevelField.writeMessage(&battLevel, fp);

        // Field 1: "battery voltage in mV"
        mFHBatteryVoltageField.writeDef(fp);
        FIT_FIELD_DESCRIPTION_MESG battVoltage{};
        strncpy(battVoltage.field_name, "battVoltage", FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT - 1);
        strncpy(battVoltage.units, "mV", FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT - 1);
        battVoltage.developer_data_index    = 0;
        battVoltage.field_definition_number = mFHBatteryVoltageField.getFieldID();
        battVoltage.fit_base_type_id        = FIT_BASE_TYPE_UINT16;
        mFHBatteryVoltageField.writeMessage(&battVoltage, fp);
    }

    mFHEvent.writeDef(fp);
    mFHActivity.writeDef(fp);

    mFHRecord.writeDef(fp);
    mFHRecordG.writeDef(fp);
    mFHRecordB.writeDef(fp);
    mFHRecordGB.writeDef(fp);

    mFHLap.writeDef(fp);
    mFHSession.writeDef(fp);

    // Write Event message - START Event
    AddMessageEvent(info.timestamp, FIT_EVENT_TYPE_START);
}

void ActivityWriter::pause(std::time_t timestamp)
{
    if (!mFile) {
        return;
    }

    // Write Event message - STOP Event
    AddMessageEvent(timestamp, FIT_EVENT_TYPE_STOP);
}

void ActivityWriter::resume(std::time_t timestamp)
{
    if (!mFile) {
        return;
    }

    // Write Event message - START Event
    AddMessageEvent(timestamp, FIT_EVENT_TYPE_START);
}

FIT_RECORD_MESG ActivityWriter::prepareRecordMsg(const RecordData& record)
{
    FIT_RECORD_MESG msg;

    Fit_InitMesg(fit_mesg_defs[FIT_MESG_RECORD], &msg);

    msg.timestamp = unixToFitTimestamp(record.timestamp);

    if (record.has(RecordData::Field::COORDS)) {
        msg.position_lat   = ConvertDegreesToSemicircles(record.latitude);
        msg.position_long  = ConvertDegreesToSemicircles(record.longitude);
    }

    if (record.has(RecordData::Field::SPEED)) {
        msg.enhanced_speed = static_cast<FIT_UINT32>(record.speed * 1000); // 1000 * m/s + 0
    }

    if (record.has(RecordData::Field::ALTITUDE)) {
        msg.enhanced_altitude = static_cast<FIT_UINT32>((record.altitude + 500) * 5);   // 5 * m + 500
    }

    if (record.has(RecordData::Field::HEART_RATE)) {
        msg.heart_rate = static_cast<FIT_UINT8>(record.heartRate);
    }

    return msg;
}

void ActivityWriter::addRecord(const RecordData& record)
{
    if (!mFile) {
        return;
    }

    const FIT_RECORD_MESG msg = prepareRecordMsg(record);

    if (record.has(RecordData::Field::BATTERY)) {
        const FIT_UINT8  soc     = record.batteryLevel;
        const FIT_UINT16 voltage = record.batteryVoltage;
        if (record.has(RecordData::Field::COORDS)) {
            mFHRecordGB.writeMessage(&msg, mFile.get());
            mFHRecordGB.writeFieldMessage(0, &soc, mFile.get());
            mFHRecordGB.writeFieldMessage(1, &voltage, mFile.get());
        } else {
            mFHRecordB.writeMessage(&msg, mFile.get());
            mFHRecordB.writeFieldMessage(0, &soc, mFile.get());
            mFHRecordGB.writeFieldMessage(1, &voltage, mFile.get());
        }
    } else {
        if (record.has(RecordData::Field::COORDS)) {
            mFHRecordG.writeMessage(&msg, mFile.get());
        } else {
            mFHRecord.writeMessage(&msg, mFile.get());
        }
    }
}

void ActivityWriter::addLap(const LapData& lap)
{
    if (!mFile) {
        return;
    }
    SDK::Interface::IFile* fp = mFile.get();

    FIT_LAP_MESG lap_mesg{};
    Fit_InitMesg(fit_mesg_defs[FIT_MESG_LAP], &lap_mesg);

    lap_mesg.message_index = 0;

    lap_mesg.timestamp = unixToFitTimestamp(lap.timestamp); // 1 * s + 0, Lap end time
    lap_mesg.start_time = unixToFitTimestamp(lap.timeStart);
    lap_mesg.total_elapsed_time = static_cast<FIT_UINT32>((lap.elapsed) * 1000); // 1000 * s + 0, Time (includes pauses)
    lap_mesg.total_timer_time = static_cast<FIT_UINT32>((lap.duration) * 1000); // 1000 * s + 0, Timer Time (excludes pauses)

    lap_mesg.total_distance = static_cast<FIT_UINT32>(lap.distance * 100); // 100 * m + 0,

    lap_mesg.avg_speed = static_cast<FIT_UINT16>(lap.speedAvg * 1000);// 1000 * m/s + 0, total_distance / total_timer_time
    lap_mesg.max_speed = static_cast<FIT_UINT16>(lap.speedMax * 1000);// 1000 * m/s + 0,

    lap_mesg.avg_heart_rate = static_cast<FIT_UINT8>(lap.hrAvg);// 1 * bpm + 0, average heart rate (excludes pause time)
    lap_mesg.max_heart_rate = static_cast<FIT_UINT8>(lap.hrMax); // 1 * bpm + 0,

    lap_mesg.total_ascent = static_cast<FIT_UINT16>(lap.ascent); // 1 * m + 0
    lap_mesg.total_descent = static_cast<FIT_UINT16>(lap.descent); // 1 * m + 0

    mFHLap.writeMessage(&lap_mesg, fp);

    mLapCounter++;
}

void ActivityWriter::stop(const TrackData& track)
{
    if (!mFile) {
        return;
    }

    SDK::Interface::IFile* fp = mFile.get();

    // Write Session message.
    {
        FIT_SESSION_MESG session_mesg{};
        Fit_InitMesg(fit_mesg_defs[FIT_MESG_SESSION], &session_mesg);

        session_mesg.message_index = 0;
        session_mesg.sport = FIT_SPORT_CYCLING;
        session_mesg.sub_sport = FIT_SUB_SPORT_GENERIC; // TODO: select correct type
        session_mesg.timestamp = unixToFitTimestamp(track.timestamp);   // 1 * s + 0, Session end time
        session_mesg.start_time = unixToFitTimestamp(track.timeStart);

        session_mesg.total_elapsed_time = static_cast<FIT_UINT32>(track.elapsed * 1000);  // 1000 * s + 0, Time (includes pauses)
        session_mesg.total_timer_time = static_cast<FIT_UINT32>(track.duration * 1000);   // 1000 * s + 0, Timer Time (excludes pauses)

        session_mesg.total_distance = static_cast<FIT_UINT32>(track.distance * 100);   // 100 * m + 0,

        session_mesg.avg_speed = static_cast<FIT_UINT16>(track.speedAvg * 1000); // 1000 * m/s + 0, total_distance / total_timer_time
        session_mesg.max_speed = static_cast<FIT_UINT16>(track.speedMax * 1000); // 1000 * m/s + 0,

        session_mesg.avg_heart_rate = static_cast<FIT_UINT8>(track.hrAvg);   // 1 * bpm + 0, average heart rate (excludes pause time)
        session_mesg.max_heart_rate = static_cast<FIT_UINT8>(track.hrMax);   // 1 * bpm + 0,

        session_mesg.total_ascent = static_cast<FIT_UINT16>(track.ascent);   // 1 * m + 0
        session_mesg.total_descent = static_cast<FIT_UINT16>(track.descent); // 1 * m + 0

        session_mesg.num_laps = mLapCounter;

        mFHSession.writeMessage(&session_mesg, fp);
    }

    // Write Activity message.
    {
        FIT_ACTIVITY_MESG activity_mesg {};

        activity_mesg.timestamp        = unixToFitTimestamp(track.timestamp);
        activity_mesg.local_timestamp  = unixToFitTimestamp(epochToLocal(track.timestamp));  // timestamp epoch expressed in local time
        activity_mesg.total_timer_time = static_cast<FIT_UINT32>(track.duration * 1000);   // 1000 * s + 0, Exclude pauses
        activity_mesg.num_sessions     = 1;

        mFHActivity.writeMessage(&activity_mesg, fp);
    }

    fp->seek(0);

    WriteFileHeader(fp);

    WriteCRC(fp);

    saveFile();

    saveSummary(track);
}

void ActivityWriter::discard()
{
    deleteFile();
}

void ActivityWriter::AddMessageEvent(std::time_t t, FIT_EVENT_TYPE type)
{
    FIT_EVENT_MESG event_mesg{};

    event_mesg.timestamp  = unixToFitTimestamp(t);
    event_mesg.event      = FIT_EVENT_TIMER;
    event_mesg.event_type = type;

    mFHEvent.writeMessage(&event_mesg, mFile.get());
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
    int len = snprintf(buff, sizeof(buff), "%s/%04u%02u/", mPath, localTime.tm_year + 1900, localTime.tm_mon + 1);
    if (len <= 0 || !mKernel.fs.mkdir(buff)) {
        LOG_ERROR("Failed to create dir [%s]\n", buff);
        return false;
    }

    // Create file 
    snprintf(&buff[len], sizeof(buff) - len, "activity_%04u%02u%02uT%02u%02u%02u.fit",
        localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
        localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

    mFile = mKernel.fs.file(buff);
    if (!mFile || !mFile->open(true, true)) { // write mode, override
        LOG_ERROR("Failed to create file [%s]\n", buff);
        mFile.reset();
        return false;
    }

    return true;
}

void ActivityWriter::saveFile()
{
    if (!mFile) {
        return;
    }

    mFile->flush();
    mFile->close();
}

void ActivityWriter::deleteFile()
{
    if (!mFile) {
        return;
    }

    if (mFile->isOpen()) {
        mFile->close();
    }

    mFile->remove();
    mFile.reset();
}

void ActivityWriter::saveSummary(const TrackData& track)
{
    char buff[256]{};
    // Create name
    size_t nameLen = strlen(mFile->getPath());
    snprintf(buff, sizeof(buff), "%.*s%s", nameLen - 3, mFile->getPath(), "json");

    mFile->setPath(buff);

    if (!mFile->open(true, true)) {
        mFile.reset();
        return;
    }

    SDK::JsonStreamWriter writer(mFile.get());

    writer.startMap();

    writer.add("time_start", static_cast<uint32_t>(track.timeStart));
    writer.add("duration", static_cast<uint32_t>(track.duration));
    writer.add("distance", track.distance);
    writer.add("hr_avg", track.hrAvg);
    writer.add("elevation", track.ascent - track.descent);
    writer.add("activity_type", "running");

    writer.endMap();

    mFile->flush();
    mFile->close();
}


std::time_t ActivityWriter::tm2epoch(const struct tm* tm)
{
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;     // 1..12
    int d = tm->tm_mday;        // 1..31

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    // Julian day
    int64_t  era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + (int64_t)doe - 719468; // 1970-01-01 is 719468
    int64_t secs = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;

    return (time_t)secs;
}

time_t ActivityWriter::epochToLocal(time_t utc)
{
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif
    return tm2epoch(&localTime);
}

FIT_DATE_TIME ActivityWriter::unixToFitTimestamp(std::time_t unixTimestamp)
{
    const std::time_t FIT_EPOCH_OFFSET = 631065600;
    return static_cast<FIT_DATE_TIME>(unixTimestamp - FIT_EPOCH_OFFSET);
}

// Convert degrees to semicircles
FIT_SINT32 ActivityWriter::ConvertDegreesToSemicircles(float degrees)
{
    return (FIT_SINT32)(degrees * (2147483648.0 / 180.0));
}


// FIT-C

void ActivityWriter::WriteFileHeader(SDK::Interface::IFile* fp)
{
    FIT_FILE_HDR file_header{};

    file_header.header_size = FIT_FILE_HDR_SIZE;
    file_header.profile_version = FIT_PROFILE_VERSION;
    file_header.protocol_version = FIT_PROTOCOL_VERSION_20;
    memcpy((FIT_UINT8*)&file_header.data_type, ".FIT", 4);

    fp->flush();
    size_t fileSize = fp->size();

    if (fileSize > FIT_FILE_HDR_SIZE) {
        file_header.data_size = static_cast<FIT_UINT32>(fileSize - FIT_FILE_HDR_SIZE);
    }
    else {
        file_header.data_size = 0;
    }

    file_header.crc = FitCRC_Calc16(&file_header, FIT_STRUCT_OFFSET(crc, FIT_FILE_HDR));

    fp->seek(0);

    size_t bw;
    fp->write(reinterpret_cast<const char*>(&file_header), FIT_FILE_HDR_SIZE, bw);

    fp->flush();

    // Move pointer to the end of the file
    if (fileSize > 0) {
        fp->seek(fileSize);
    }
}

void ActivityWriter::WriteCRC(SDK::Interface::IFile* fp)
{
    fp->close();

    fp->open(false);

    FIT_UINT8 buffer[512];
    size_t    size = fp->size();
    size_t    pos = 0;
    uint16_t  crc = 0;

    while (pos < size) {
        size_t toRead = size - pos;
        if (toRead > sizeof(buffer)) {
            toRead = sizeof(buffer);
        }

        size_t br;
        fp->read(reinterpret_cast<char*>(buffer), toRead, br);

        crc = FitCRC_Update16(crc, buffer, static_cast<FIT_UINT32>(br));

        pos += br;
    }

    fp->close();

    fp->open(true, false);

    fp->seek(fp->size());

    size_t bw;
    fp->write(reinterpret_cast<const char*>(&crc), sizeof(FIT_UINT16), bw);
    fp->flush();
}

