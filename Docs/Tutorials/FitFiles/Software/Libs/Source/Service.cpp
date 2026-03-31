#include "Service.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"
#include <array>
#include <memory>
#include <cstring>
#include <algorithm>

#define LOG_MODULE_PRX "Service"
#define LOG_MODULE_LEVEL LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"

extern "C" {
#include "fit_product.h"
#include "fit_crc.h"
}

#ifndef DEV_ID
#define DEV_ID "UNA"
#endif

#ifndef APP_ID
#define APP_ID "STEPS"
#endif

namespace {
// ===== FIT UTILITIES =====
constexpr std::time_t kFitEpochOffset = 631065600;

// Convert Unix timestamp to FIT timestamp
static FIT_DATE_TIME unixToFitTimestamp(std::time_t unixTimestamp) {
    return static_cast<FIT_DATE_TIME>(unixTimestamp - kFitEpochOffset);
}

// Write FIT file header with correct data size
static void writeFileHeader(SDK::Interface::IFile* fp) {
    FIT_FILE_HDR file_header{};

    file_header.header_size = FIT_FILE_HDR_SIZE;
    file_header.profile_version = FIT_PROFILE_VERSION;
    file_header.protocol_version = FIT_PROTOCOL_VERSION_20;
    std::memcpy(reinterpret_cast<FIT_UINT8*>(&file_header.data_type), ".FIT", 4);

    fp->flush();
    size_t fileSize = fp->size();

    size_t dataSize = 0;
    if (fileSize > FIT_FILE_HDR_SIZE) {
        dataSize = fileSize - FIT_FILE_HDR_SIZE;
    }
    file_header.data_size = static_cast<FIT_UINT32>(dataSize);

    file_header.crc = FitCRC_Calc16(&file_header, FIT_STRUCT_OFFSET(crc, FIT_FILE_HDR));

    fp->seek(0);
    size_t bw;
    fp->write(reinterpret_cast<const char*>(&file_header), FIT_FILE_HDR_SIZE, bw);
    fp->flush();

    if (fileSize > 0) {
        fp->seek(fileSize);
    }
}

// Calculate and append CRC to FIT file
static bool writeCRC(SDK::Interface::IFile* fp) {
    FIT_UINT8 buffer[512];
    fp->flush();
    size_t sizeBefore = fp->size();
    size_t pos = 0;
    uint16_t crc = 0;

    fp->seek(0);

    while (pos < sizeBefore) {
        size_t toRead = sizeBefore - pos;
        if (toRead > sizeof(buffer)) {
            toRead = sizeof(buffer);
        }

        size_t br;
        fp->read(reinterpret_cast<char*>(buffer), toRead, br);
        if (br == 0) {
            LOG_ERROR("writeCRC read failed at position %zu\n", pos);
            return false;
        }
        crc = FitCRC_Update16(crc, buffer, static_cast<FIT_UINT32>(br));
        pos += br;
    }

    fp->seek(sizeBefore);
    size_t bw;
    fp->write(reinterpret_cast<const char*>(&crc), sizeof(FIT_UINT16), bw);
    if (bw != sizeof(FIT_UINT16)) {
        LOG_ERROR("writeCRC write failed\n");
        return false;
    }
    fp->flush();
    return true;
}

}  // namespace

#include "icon_60x60.h"
#include "icon_30x30.h"

// ===== CONSTRUCTOR =====
Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel),
      mGlanceUI(),
      mGlanceTitle(),
      mGlanceValue(),
      mGlanceHR(),
      mSensorSteps(SDK::Sensor::Type::STEP_COUNTER),
      mSensorHR(SDK::Sensor::Type::HEART_RATE),
      // Initialize FIT helpers for different message types
      mFitFileID(skFileMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_FILE_ID]),
      mFitDeveloper(skDevelopMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_DEVELOPER_DATA_ID]),
      mFitRecord(skRecordMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_RECORD]),
      mFitEvent(skEventMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_EVENT]),
      mFitSession(skSessionMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_SESSION]),
      mFitActivity(skActivityMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_ACTIVITY]),
      mFitStepsField(skStepsMsgNum, 0, {&mFitRecord}) {

    // Initialize standard FIT message helpers
    mFitFileID.init();
    mFitDeveloper.init();
    mFitEvent.init({FIT_EVENT_FIELD_NUM_TIMESTAMP, FIT_EVENT_FIELD_NUM_EVENT, FIT_EVENT_FIELD_NUM_EVENT_TYPE});
    mFitActivity.init({FIT_ACTIVITY_FIELD_NUM_TIMESTAMP, FIT_ACTIVITY_FIELD_NUM_TOTAL_TIMER_TIME,
                      FIT_ACTIVITY_FIELD_NUM_LOCAL_TIMESTAMP, FIT_ACTIVITY_FIELD_NUM_NUM_SESSIONS});

    // Initialize Record message with timestamp and heart rate fields
    mFitRecord.init({FIT_RECORD_FIELD_NUM_TIMESTAMP, FIT_RECORD_FIELD_NUM_HEART_RATE});

    // Initialize Session message with essential fields
    mFitSession.init({FIT_SESSION_FIELD_NUM_TIMESTAMP, FIT_SESSION_FIELD_NUM_START_TIME,
                     FIT_SESSION_FIELD_NUM_TOTAL_ELAPSED_TIME, FIT_SESSION_FIELD_NUM_TOTAL_TIMER_TIME,
                     FIT_SESSION_FIELD_NUM_MESSAGE_INDEX, FIT_SESSION_FIELD_NUM_SPORT, FIT_SESSION_FIELD_NUM_SUB_SPORT});

    // Initialize developer field for steps
    mFitStepsField.init({FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME, FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID});
}

Service::~Service() {}

// ===== MAIN RUN LOOP =====
void Service::run() {
    LOG_INFO("Service started\n");

    // Initialize glance UI
    if (!configGui()) {
        LOG_ERROR("Can't create glance GUI\n");
        return;
    }
    createGuiControls();
    connect();

    // Main message processing loop
    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (!mKernel.comm.getMessage(msg)) {
            continue;
        }

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_GLANCE_START:
                LOG_INFO("Glance started - beginning FIT session\n");
                mGlanceActive = true;
                if (!mSessionOpen) {
                    startSession();
                }
                break;

            case SDK::MessageType::EVENT_GLANCE_STOP:
                LOG_INFO("Glance stopped - finalizing FIT session\n");
                mGlanceActive = false;
                if (mSessionOpen) {
                    finalizeSession();
                }
                break;

            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Application stopping\n");
                disconnect();
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_GLANCE_TICK:
                onGlanceTick();
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                onSdlNewData(event->handle, event->data, event->count, event->stride);
            } break;

            default:
                break;
        }

        mKernel.comm.releaseMessage(msg);
    }
}

// ===== SESSION MANAGEMENT =====
void Service::startSession() {
    std::time_t now = std::time(nullptr);
    mSessionStart = now;
    mSessionOpen = true;
    mFitFileInitialized = false;  // Will initialize on first save
    mTotalSteps = 0;
    mLastSteps = 0;
    mSampleCount = 0;
    mCurrentHR = 0;
    mPendingRecords.clear();
    LOG_INFO("FIT session started at %ld\n", now);
}

void Service::finalizeSession() {
    if (mSessionOpen) {
        saveFit(true);
        mSessionOpen = false;
        LOG_INFO("FIT session finalized\n");
    }
}

// ===== SENSOR MANAGEMENT =====
void Service::connect() {
    const float samplePeriodMs = static_cast<float>(skSamplePeriodSec) * 1000.0f;
    if (!mSensorSteps.isConnected()) {
        LOG_DEBUG("Connecting to Steps sensor\n");
        mSensorSteps.connect(samplePeriodMs);
    }
    if (!mSensorHR.isConnected()) {
        LOG_DEBUG("Connecting to HR sensor\n");
        mSensorHR.connect(samplePeriodMs);
    }
}

void Service::disconnect() {
    LOG_DEBUG("Disconnecting from sensors\n");
    finalizeSession();
    if (mSensorSteps.isConnected()) {
        mSensorSteps.disconnect();
    }
    if (mSensorHR.isConnected()) {
        mSensorHR.disconnect();
    }
}

// ===== SENSOR DATA PROCESSING =====
void Service::onSdlNewData(uint16_t handle, const SDK::Sensor::Data* data, uint16_t count, uint16_t stride) {
    if (!mSessionOpen) return;  // Only process data during active sessions

    std::time_t now = std::time(nullptr);
    SDK::Sensor::DataBatch batch(data, count, stride);
    bool hasNewData = false;

    // Process step counter data
    if (mSensorSteps.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::StepCounter p(batch[i]);
            if (!p.isDataValid()) continue;
            uint32_t steps = p.getStepCount();
            if (steps > mLastSteps) {
                uint32_t delta = steps - mLastSteps;
                mTotalSteps += delta;
                mLastSteps = steps;
                mSampleCount++;
                hasNewData = true;
            }
        }
    }
    // Process heart rate data
    else if (mSensorHR.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::HeartRate p(batch[i]);
            if (!p.isDataValid()) continue;
            uint8_t newHR = static_cast<uint8_t>(p.getBpm());
            if (newHR > 0) {  // Only consider valid HR readings
                mCurrentHR = newHR;
                hasNewData = true;
            }
        }
    }

    // Accumulate records for FIT file if we have new valid data
    if (hasNewData) {
        mPendingRecords.push_back({now, mCurrentHR, mTotalSteps});
        LOG_DEBUG("Recorded data point: HR=%u, steps=%u\n", mCurrentHR, mTotalSteps);
    }
}

void Service::onGlanceTick() {
    mGlanceValue.print("%u", mTotalSteps);
    mGlanceHR.print("HR: %u", mCurrentHR);

    if (mGlanceUI.isInvalid()) {
        auto* upd = mKernel.comm.allocateMessage<SDK::Message::RequestGlanceUpdate>();
        if (upd) {
            upd->name = "FitTutorial";
            upd->controls = mGlanceUI.data();
            upd->controlsNumber = static_cast<uint32_t>(mGlanceUI.size());

            mKernel.comm.sendMessage(upd, 100);
            mKernel.comm.releaseMessage(upd);
        }

        mGlanceUI.setValid();
    }
}

bool Service::configGui() {
    bool status = false;
    auto* gc = mKernel.comm.allocateMessage<SDK::Message::RequestGlanceConfig>();
    if (gc) {
        if (mKernel.comm.sendMessage(gc, 100) && gc->getResult() == SDK::MessageResult::SUCCESS) {
            if (gc->maxControls >= 4) {
                mGlanceUI.setWidth(gc->width);
                mGlanceUI.setHeight(gc->height);
                status = true;
            }
        }
        mKernel.comm.releaseMessage(gc);
    }

    return status;
}

void Service::createGuiControls() {
    mGlanceUI.createImage().init({31, 15}, {60, 60}, ICON_60X60_ABGR2222);

    mGlanceTitle = mGlanceUI.createText();
    mGlanceTitle.pos({20, 0}, {200, 25})
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_TEAL)
        .setText("Steps")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    mGlanceValue = mGlanceUI.createText();
    mGlanceValue.pos({80, 28}, {80, 34})
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .setText("")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    mGlanceHR = mGlanceUI.createText();
    mGlanceHR.pos({20, 65}, {180, 25})
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .setText("")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);
}

// ===== FIT FILE MANAGEMENT =====
void Service::saveFit(bool finalize) {
    if (!mSessionOpen) return;

    LOG_DEBUG("Saving FIT file (finalize=%d)\n", finalize);

    // Open FIT file for writing
    auto file = mKernel.fs.file(skFitFileName);
    if (!file) {
        LOG_ERROR("Cannot access FIT file\n");
        return;
    }

    bool isNewFile = !file->exist();
    if (!file->open(true, isNewFile)) {
        LOG_ERROR("Failed to open FIT file\n");
        return;
    }

    std::time_t now = std::time(nullptr);

    // If new file, write definitions and start session
    if (isNewFile || !mFitFileInitialized) {
        LOG_DEBUG("Writing FIT file definitions\n");
        writeFitDefinitions(file.get(), mSessionStart);
        mFitFileInitialized = true;
    } else {
        // Seek to end for appending
        file->seek(file->size());
    }

    // Append pending records
    LOG_DEBUG("Appending %zu pending records\n", mPendingRecords.size());
    appendPendingRecords(file.get());

    // If finalizing session, write session summary
    if (finalize) {
        LOG_DEBUG("Writing session summary\n");
        writeFitSessionSummary(file.get(), now);
    }

    // Update header and write CRC
    file->flush();
    writeFileHeader(file.get());
    if (!writeCRC(file.get())) {
        LOG_ERROR("Failed to write CRC to FIT file\n");
    }
    file->close();

    LOG_INFO("FIT file saved: %s\n", skFitFileName);
}

// ===== FIT FILE OPERATIONS =====
void Service::writeFitDefinitions(SDK::Interface::IFile* fp, std::time_t timestamp) {
    // Write file header placeholder
    writeFileHeader(fp);

    // File ID message - identifies the file
    mFitFileID.writeDef(fp);
    FIT_FILE_ID_MESG file_id_mesg{};
    std::strncpy(file_id_mesg.product_name, "UNA Watch", FIT_FILE_ID_MESG_PRODUCT_NAME_COUNT);
    file_id_mesg.serial_number = 0;
    file_id_mesg.time_created = unixToFitTimestamp(timestamp);
    file_id_mesg.manufacturer = FIT_MANUFACTURER_DEVELOPMENT;
    file_id_mesg.product = 0;
    file_id_mesg.number = 0;
    file_id_mesg.type = FIT_FILE_ACTIVITY;
    mFitFileID.writeMessage(&file_id_mesg, fp);

    // Developer Data ID - registers developer for custom fields
    mFitDeveloper.writeDef(fp);
    FIT_DEVELOPER_DATA_ID_MESG developer{};
    size_t devIdLen = std::min(std::strlen(DEV_ID), static_cast<size_t>(FIT_DEVELOPER_DATA_ID_MESG_DEVELOPER_ID_COUNT - 1));
    std::memcpy(developer.developer_id, DEV_ID, devIdLen);
    developer.developer_id[devIdLen] = '\0';
    size_t appIdLen = std::min(std::strlen(APP_ID), static_cast<size_t>(FIT_DEVELOPER_DATA_ID_MESG_APPLICATION_ID_COUNT - 1));
    std::memcpy(developer.application_id, APP_ID, appIdLen);
    developer.application_id[appIdLen] = '\0';
    developer.application_version = SDK::ParseVersion(BUILD_VERSION).u32;
    developer.manufacturer_id = FIT_MANUFACTURER_DEVELOPMENT;
    developer.developer_data_index = 0;
    mFitDeveloper.writeMessage(&developer, fp);

    // Field Description for steps developer field
    mFitStepsField.writeDef(fp);
    FIT_FIELD_DESCRIPTION_MESG stepsField{};
    std::strncpy(stepsField.field_name, "steps", FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT);
    std::strncpy(stepsField.units, "count", FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT);
    stepsField.developer_data_index = 0;
    stepsField.field_definition_number = 0;
    stepsField.fit_base_type_id = FIT_BASE_TYPE_UINT32;
    mFitStepsField.writeMessage(&stepsField, fp);

    // Write definitions for all message types
    mFitEvent.writeDef(fp);
    mFitRecord.writeDef(fp);
    mFitSession.writeDef(fp);
    mFitActivity.writeDef(fp);

    // Start session event
    FIT_EVENT_MESG start_event{};
    start_event.timestamp = unixToFitTimestamp(timestamp);
    start_event.event = FIT_EVENT_TIMER;
    start_event.event_type = FIT_EVENT_TYPE_START;
    mFitEvent.writeMessage(&start_event, fp);
}

void Service::appendPendingRecords(SDK::Interface::IFile* fp) {
    if (mPendingRecords.empty()) return;

    // Write each pending record
    for (const auto& rec : mPendingRecords) {
        FIT_RECORD_MESG record_mesg{};
        record_mesg.timestamp = unixToFitTimestamp(rec.timestamp);
        record_mesg.heart_rate = rec.heartRate;
        mFitRecord.writeMessage(&record_mesg, fp);

        // Write developer field for steps
        uint32_t steps = rec.steps;
        mFitRecord.writeFieldMessage(0, &steps, fp);
    }

    mPendingRecords.clear();
}

void Service::writeFitSessionSummary(SDK::Interface::IFile* fp, std::time_t timestamp) {
    // Stop session event
    FIT_EVENT_MESG stop_event{};
    stop_event.timestamp = unixToFitTimestamp(timestamp);
    stop_event.event = FIT_EVENT_TIMER;
    stop_event.event_type = FIT_EVENT_TYPE_STOP;
    mFitEvent.writeMessage(&stop_event, fp);

    // Session summary
    FIT_SESSION_MESG session_mesg{};
    session_mesg.message_index = 0;
    session_mesg.sport = FIT_SPORT_GENERIC;
    session_mesg.sub_sport = FIT_SUB_SPORT_GENERIC;
    session_mesg.timestamp = unixToFitTimestamp(timestamp);
    session_mesg.start_time = unixToFitTimestamp(mSessionStart);
    session_mesg.total_elapsed_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    session_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    mFitSession.writeMessage(&session_mesg, fp);

    // Activity summary
    FIT_ACTIVITY_MESG activity_mesg{};
    activity_mesg.timestamp = unixToFitTimestamp(timestamp);
    activity_mesg.local_timestamp = unixToFitTimestamp(timestamp);  // Simplified
    activity_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    activity_mesg.num_sessions = 1;
    mFitActivity.writeMessage(&activity_mesg, fp);
}