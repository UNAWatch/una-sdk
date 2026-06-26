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

namespace fit = SDK::Fit;

#ifndef DEV_ID
#define DEV_ID "UNA"
#endif

#ifndef APP_ID
#define APP_ID "STEPS"
#endif

namespace {
// ===== FIT UTILITIES =====
constexpr std::time_t kFitEpochOffset = 631065600;

// Convert Unix timestamp to FIT timestamp (seconds since the FIT epoch).
static uint32_t unixToFitTimestamp(std::time_t unixTimestamp) {
    return static_cast<uint32_t>(unixTimestamp - kFitEpochOffset);
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
      mSensorHR(SDK::Sensor::Type::HEART_RATE) {
    // FIT messages are defined on the encoder when the file is opened (saveFit).
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

    // Open FIT file for writing. The native encoder streams the whole activity
    // in one pass, so the file is (re)created and written end-to-end here.
    auto file = mKernel.fs.file(skFitFileName);
    if (!file) {
        LOG_ERROR("Cannot access FIT file\n");
        return;
    }

    if (!file->open(true, true)) {
        LOG_ERROR("Failed to open FIT file\n");
        return;
    }

    std::time_t now = std::time(nullptr);

    // Construct the encoder over the open file and write the header placeholder.
    mFit = std::make_unique<fit::FitWriter>(*file);
    mFit->begin(/*profileVersion=*/0);

    // Definitions + start event.
    LOG_DEBUG("Writing FIT file definitions\n");
    writeFitDefinitions(mSessionStart);

    // Records accumulated this session.
    LOG_DEBUG("Appending %zu pending records\n", mPendingRecords.size());
    appendPendingRecords();

    // Session summary (stop event + session + activity).
    if (finalize) {
        LOG_DEBUG("Writing session summary\n");
        writeFitSessionSummary(now);
    }

    // finish() back-patches the header data size + CRC and appends the file CRC.
    mFit->finish();
    mFit.reset();

    file->flush();
    file->close();

    LOG_INFO("FIT file saved: %s\n", skFitFileName);
}

// ===== FIT FILE OPERATIONS =====
void Service::writeFitDefinitions(std::time_t timestamp) {
    // File ID message - identifies the file.
    mFit->defineMessage(L_FILE_ID, fit::mesgNum(fit::MesgNum::FileId),
        {fit::field::FileId::Type, fit::field::FileId::Manufacturer,
         fit::field::FileId::Product, fit::field::FileId::SerialNumber,
         fit::field::FileId::TimeCreated});
    mFit->data(L_FILE_ID)
        .u8(static_cast<uint8_t>(fit::File::Activity))
        .u16(static_cast<uint16_t>(fit::Manufacturer::Development))
        .u16(0)  // product
        .u32(0)  // serial_number
        .u32(unixToFitTimestamp(timestamp))
        .write();

    // Developer Data ID - registers the developer for custom fields.
    mFit->defineMessage(L_DEV_ID, fit::mesgNum(fit::MesgNum::DeveloperDataId),
        {fit::field::DeveloperDataId::ApplicationId,
         fit::field::DeveloperDataId::DeveloperDataIndex});
    {
        uint8_t appId[16] = {};
        std::strncpy(reinterpret_cast<char*>(appId), APP_ID, sizeof(appId));
        mFit->data(L_DEV_ID).bytes(appId, sizeof(appId)).u8(0).write();
    }

    // Field Description for the "steps" developer field.
    writeStepsFieldDescription();

    // Event / Record / Session / Activity definitions.
    mFit->defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});

    // Record carries timestamp + heart_rate, plus the "steps" developer field
    // (dev field number, size in bytes, developer-data index 0).
    mFit->defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp, fit::field::Record::HeartRate},
        {{skStepsDevFieldNum, fit::baseTypeSize(fit::BaseType::UInt32), 0}});

    mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
        {fit::field::Session::Timestamp, fit::field::Session::StartTime,
         fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
         fit::field::Session::MessageIndex, fit::field::Session::Sport,
         fit::field::Session::SubSport});

    mFit->defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
        {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
         fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});

    // Start session event.
    mFit->data(L_EVENT)
        .u32(unixToFitTimestamp(timestamp))
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(fit::EventType::Start))
        .write();
}

void Service::writeStepsFieldDescription() {
    // field_description: name/units survive any profile; size the strings exactly.
    const char* name  = "steps";
    const char* units = "count";
    const uint8_t nameLen  = static_cast<uint8_t>(std::strlen(name) + 1);
    const uint8_t unitsLen = static_cast<uint8_t>(std::strlen(units) + 1);

    mFit->defineMessage(L_FIELD_DESC, fit::mesgNum(fit::MesgNum::FieldDescription),
        {fit::field::FieldDescription::DeveloperDataIndex,
         fit::field::FieldDescription::FieldDefinitionNumber,
         fit::field::FieldDescription::FitBaseTypeId,
         {fit::field::FieldDescription::kFieldNameNum, fit::BaseType::String, nameLen},
         {fit::field::FieldDescription::kUnitsNum, fit::BaseType::String, unitsLen}});
    mFit->data(L_FIELD_DESC)
        .u8(0)                  // developer_data_index
        .u8(skStepsDevFieldNum) // field_definition_number
        .u8(fit::baseTypeId(fit::BaseType::UInt32))
        .str(name, nameLen)
        .str(units, unitsLen)
        .write();
}

void Service::appendPendingRecords() {
    if (mPendingRecords.empty()) return;

    // Write each pending record: timestamp + heart_rate, then the steps
    // developer field (definition order, dev field last).
    for (const auto& rec : mPendingRecords) {
        mFit->data(L_RECORD)
            .u32(unixToFitTimestamp(rec.timestamp))
            .u8(rec.heartRate)
            .u32(rec.steps)
            .write();
    }

    mPendingRecords.clear();
}

void Service::writeFitSessionSummary(std::time_t timestamp) {
    // Stop session event.
    mFit->data(L_EVENT)
        .u32(unixToFitTimestamp(timestamp))
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(fit::EventType::Stop))
        .write();

    const uint32_t elapsedMs = static_cast<uint32_t>((timestamp - mSessionStart) * 1000);

    // Session summary.
    mFit->data(L_SESSION)
        .u32(unixToFitTimestamp(timestamp))
        .u32(unixToFitTimestamp(mSessionStart))
        .u32(elapsedMs)  // total_elapsed_time, scale 1000
        .u32(elapsedMs)  // total_timer_time, scale 1000
        .u16(0)          // message_index
        .u8(static_cast<uint8_t>(fit::Sport::Generic))
        .u8(static_cast<uint8_t>(fit::SubSport::Generic))
        .write();

    // Activity summary.
    mFit->data(L_ACTIVITY)
        .u32(unixToFitTimestamp(timestamp))
        .u32(elapsedMs)  // total_timer_time, scale 1000
        .u32(unixToFitTimestamp(timestamp))  // local_timestamp (simplified)
        .u16(1)          // num_sessions
        .write();
}