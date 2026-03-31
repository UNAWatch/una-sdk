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
constexpr std::time_t kFitEpochOffset = 631065600;

static std::time_t tm2epoch(const std::tm* tm) {
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    int64_t era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = static_cast<uint32_t>(y - era * 400);
    uint32_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + static_cast<int64_t>(doe) - 719468;
    int64_t secs = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;

    return static_cast<std::time_t>(secs);
}

static std::time_t epochToLocal(std::time_t utc) {
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif
    return tm2epoch(&localTime);
}

static FIT_DATE_TIME unixToFitTimestamp(std::time_t unixTimestamp) {
    return static_cast<FIT_DATE_TIME>(unixTimestamp - kFitEpochOffset);
}

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
    LOG_DEBUG("writeFileHeader size=%zu dataSize=%zu header=%u\n", fileSize, dataSize,
              static_cast<unsigned>(FIT_FILE_HDR_SIZE));
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

static void writeCRC(SDK::Interface::IFile* fp) {
    FIT_UINT8 buffer[512];
    fp->flush();
    size_t sizeBefore = fp->size();
    size_t pos = 0;
    uint16_t crc = 0;

    LOG_DEBUG("writeCRC sizeBefore=%zu\n", sizeBefore);
    fp->seek(0);

    while (pos < sizeBefore) {
        size_t toRead = sizeBefore - pos;
        if (toRead > sizeof(buffer)) {
            toRead = sizeof(buffer);
        }

        size_t br;
        fp->read(reinterpret_cast<char*>(buffer), toRead, br);
        if (br == 0) {
            LOG_ERROR("writeCRC read failed at pos=%zu sizeBefore=%zu\n", pos, sizeBefore);
            break;
        }
        crc = FitCRC_Update16(crc, buffer, static_cast<FIT_UINT32>(br));
        pos += br;
    }

    fp->seek(sizeBefore);
    LOG_DEBUG("writeCRC append crc=0x%04x at pos=%zu\n", crc, sizeBefore);

    size_t bw;
    fp->write(reinterpret_cast<const char*>(&crc), sizeof(FIT_UINT16), bw);
    fp->flush();
}

}  // namespace

#include "icon_60x60.h"
#include "icon_30x30.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel),
      mName("Steps"),
      mGlanceUI(),
      mGlanceTitle(),
      mGlanceValue(),
      mGlanceHR(),
      mSensorSteps(SDK::Sensor::Type::STEP_COUNTER),
      mSensorHR(SDK::Sensor::Type::HEART_RATE),
      mFitFileID(skFileMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_FILE_ID]),
      mFitDeveloper(skDevelopMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_DEVELOPER_DATA_ID]),
      mFitRecord(skRecordMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_RECORD]),
      mFitEvent(skEventMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_EVENT]),
      mFitSession(skSessionMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_SESSION]),
      mFitActivity(skActivityMsgNum, (FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_ACTIVITY]),
      mFitStepsField(skStepsMsgNum, 0, {&mFitRecord}) {
    mFitFileID.init();
    mFitDeveloper.init();
    mFitEvent.init({FIT_EVENT_FIELD_NUM_TIMESTAMP, FIT_EVENT_FIELD_NUM_EVENT, FIT_EVENT_FIELD_NUM_EVENT_TYPE});
    mFitActivity.init({FIT_ACTIVITY_FIELD_NUM_TIMESTAMP, FIT_ACTIVITY_FIELD_NUM_TOTAL_TIMER_TIME,
                      FIT_ACTIVITY_FIELD_NUM_LOCAL_TIMESTAMP, FIT_ACTIVITY_FIELD_NUM_NUM_SESSIONS});
    mFitRecord.init({FIT_RECORD_FIELD_NUM_TIMESTAMP, FIT_RECORD_FIELD_NUM_HEART_RATE});
    mFitSession.init({FIT_SESSION_FIELD_NUM_TIMESTAMP, FIT_SESSION_FIELD_NUM_START_TIME,
                     FIT_SESSION_FIELD_NUM_TOTAL_ELAPSED_TIME, FIT_SESSION_FIELD_NUM_TOTAL_TIMER_TIME,
                     FIT_SESSION_FIELD_NUM_MESSAGE_INDEX, FIT_SESSION_FIELD_NUM_SPORT, FIT_SESSION_FIELD_NUM_SUB_SPORT});
    mFitStepsField.init({FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME, FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER,
                        FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID});
}

Service::~Service() {}

void Service::run() {
    LOG_INFO("Started\n");

    if (!configGui()) {
        LOG_ERROR("Can't create glance-gui\n");
        return;
    }
    createGuiControls();
    connect();

    while (true) {
        SDK::MessageBase* msg = {nullptr};
        if (!mKernel.comm.getMessage(msg)) {
            continue;
        }

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_GLANCE_START:
                LOG_INFO("GLANCE is now running - starting FIT recording\n");
                mGlanceActive = true;
                // Start a new session when glance starts
                if (!mSessionOpen) {
                    checkDayRollover();
                    saveFit(false, false); // Ensure any pending data is saved
                    // startNewSession will be called in saveFit if needed
                }
                break;

            case SDK::MessageType::EVENT_GLANCE_STOP:
                LOG_INFO("GLANCE has stopped - stopping FIT recording\n");
                mGlanceActive = false;
                // Stop and save the session when glance stops
                if (mSessionOpen) {
                    std::time_t now = std::time(nullptr);
                    saveFit(true, false); // Force save with session summary
                }
                break;

            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                disconnect();
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_GLANCE_TICK:
                LOG_DEBUG("Glance tick event\n");
                onGlanceTick();
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                LOG_DEBUG("Sensor data event\n");
                auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                this->onSdlNewData(event->handle, event->data, event->count, event->stride);
            } break;

            default:
                break;
        }

        mKernel.comm.releaseMessage(msg);
    }
}

void Service::connect() {
    const float samplePeriodMs = static_cast<float>(skSamplePeriodSec) * 1000.0f;
    if (!mSensorSteps.isConnected()) {
        LOG_DEBUG("Connect to Steps sensor...\n");
        mSensorSteps.connect(samplePeriodMs);
    }
    if (!mSensorHR.isConnected()) {
        LOG_DEBUG("Connect to HR sensor...\n");
        mSensorHR.connect(samplePeriodMs);
    }
}

void Service::disconnect() {
    LOG_DEBUG("Disconnect from sensors...\n");
    saveFit(true, true);
    if (mSensorSteps.isConnected()) {
        mSensorSteps.disconnect();
    }
    if (mSensorHR.isConnected()) {
        mSensorHR.disconnect();
    }
}

void Service::onSdlNewData(uint16_t handle, const SDK::Sensor::Data* data, uint16_t count, uint16_t stride) {
    std::time_t now = std::time(nullptr);
    checkDayRollover();

    SDK::Sensor::DataBatch batch(data, count, stride);

    if (mSensorSteps.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::StepCounter p(batch[i]);
            if (!p.isDataValid()) continue;
            uint32_t steps = p.getStepCount();
            LOG_DEBUG("Steps: %u\n", steps);
            if (steps > mLastSteps) {
                uint32_t delta = steps - mLastSteps;
                mTotalSteps += delta;
                mLastSteps = steps;
                mSampleCount++;
            }
        }
    } else if (mSensorHR.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::HeartRate p(batch[i]);
            if (!p.isDataValid()) continue;
            mCurrentHR = static_cast<uint8_t>(p.getBpm());
            LOG_DEBUG("HR: %u\n", mCurrentHR);
        }
    }

    // Record data periodically if session is active and we have any sensor data
    if (mSessionOpen && (mSampleCount > 0 || mCurrentHR > 0)) {
        mPendingRecords.push_back({now, mCurrentHR, mTotalSteps});
        LOG_DEBUG("Recorded: HR=%u, steps=%u\n", mCurrentHR, mTotalSteps);
    }

    if ((now - mLastSaveTime) >= skSaveIntervalSec) {
        saveFit(false, false);
    }
}

void Service::onGlanceTick() {
    mGlanceValue.print("%u", mTotalSteps);
    mGlanceHR.print("HR: %u", mCurrentHR);

    if (mGlanceUI.isInvalid()) {
        auto* upd = mKernel.comm.allocateMessage<SDK::Message::RequestGlanceUpdate>();
        if (upd) {
            upd->name = mName;
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

void Service::checkDayRollover() {
    std::time_t t_now = std::time(nullptr);
    std::tm tm_now;
    localtime_r(&t_now, &tm_now);
    char now_date[11];
    std::strftime(now_date, sizeof(now_date), "%Y-%m-%d", &tm_now);

    if (std::strlen(mCurrentDate) == 0 || std::strcmp(mCurrentDate, now_date) != 0) {
        LOG_DEBUG("Date changed from '%s' to '%s'\n", mCurrentDate, now_date);
        if (std::strlen(mCurrentDate) > 0) {
            saveFit(true, true);
        }
        std::strncpy(mCurrentDate, now_date, 10);
        mCurrentDate[10] = '\0';
        std::snprintf(mFitPath, sizeof(mFitPath), "steps_%s.fit", mCurrentDate);

        mTotalSteps = 0;
        mLastSteps = 0;
        mSampleCount = 0;
        mCurrentHR = 0;
        mPendingRecords.clear();
        mLastSaveTime = 0;
        mDayStart = t_now;
        mFitFileInitialized = false;
        mSessionOpen = false;
        mSessionIndexInitialized = false;
        mSessionIndex = 0;

        LOG_INFO("New day: %s\n", mCurrentDate);
    }
}

void Service::writeFitDefinitions(SDK::Interface::IFile* fp, std::time_t timestamp) {
    writeFileHeader(fp);

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

    mFitDeveloper.writeDef(fp);
    FIT_DEVELOPER_DATA_ID_MESG developer{};
    std::strncpy(reinterpret_cast<char*>(developer.developer_id), DEV_ID,
                 FIT_DEVELOPER_DATA_ID_MESG_DEVELOPER_ID_COUNT);
    std::strncpy(reinterpret_cast<char*>(developer.application_id), APP_ID,
                 FIT_DEVELOPER_DATA_ID_MESG_APPLICATION_ID_COUNT);
    developer.application_version = SDK::ParseVersion(BUILD_VERSION).u32;
    developer.manufacturer_id = FIT_MANUFACTURER_DEVELOPMENT;
    developer.developer_data_index = 0;
    mFitDeveloper.writeMessage(&developer, fp);

    mFitEvent.writeDef(fp);
    mFitActivity.writeDef(fp);

    mFitStepsField.writeDef(fp);
    FIT_FIELD_DESCRIPTION_MESG stepsField{};
    std::strncpy(stepsField.field_name, "steps", FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT);
    std::strncpy(stepsField.units, "count", FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT);
    stepsField.developer_data_index = 0;
    stepsField.field_definition_number = 0;
    stepsField.fit_base_type_id = FIT_BASE_TYPE_UINT32;
    mFitStepsField.writeMessage(&stepsField, fp);

    mFitRecord.writeDef(fp);
    mFitSession.writeDef(fp);

    FIT_EVENT_MESG start_event{};
    start_event.timestamp = unixToFitTimestamp(timestamp);
    start_event.event = FIT_EVENT_TIMER;
    start_event.event_type = FIT_EVENT_TYPE_START;
    mFitEvent.writeMessage(&start_event, fp);
}

void Service::startNewSession(SDK::Interface::IFile* fp, std::time_t timestamp) {
    FIT_EVENT_MESG start_event{};
    start_event.timestamp = unixToFitTimestamp(timestamp);
    start_event.event = FIT_EVENT_TIMER;
    start_event.event_type = FIT_EVENT_TYPE_START;
    mFitEvent.writeMessage(&start_event, fp);
    mDayStart = timestamp;
    mSessionOpen = true;
}

void Service::loadSessionIndex(SDK::Interface::IFile* fp) {
    if (!fp) {
        return;
    }

    fp->flush();
    size_t fileSize = fp->size();
    if (fileSize < 12) {
        return;
    }

    FIT_UINT8 headerSize = 0;
    size_t br = 0;
    fp->seek(0);
    fp->read(reinterpret_cast<char*>(&headerSize), sizeof(FIT_UINT8), br);
    if (br != sizeof(FIT_UINT8) || headerSize < 12) {
        return;
    }

    std::array<FIT_UINT8, 32> header{};
    size_t toRead = static_cast<size_t>(headerSize - 1);
    if (toRead > header.size()) {
        toRead = header.size();
    }
    fp->read(reinterpret_cast<char*>(header.data()), toRead, br);
    if (br != toRead) {
        return;
    }

    const size_t dataSize = static_cast<size_t>(header[3]) |
                           (static_cast<size_t>(header[4]) << 8) |
                           (static_cast<size_t>(header[5]) << 16) |
                           (static_cast<size_t>(header[6]) << 24);
    if (dataSize == 0) {
        return;
    }

    size_t dataStart = headerSize;
    size_t dataEnd = dataStart + dataSize;
    if (dataEnd > fileSize) {
        dataEnd = fileSize;
    }

    struct FitFieldDef {
        FIT_UINT8 fieldNum;
        FIT_UINT8 size;
        FIT_UINT8 baseType;
    };

    struct FitDevFieldDef {
        FIT_UINT8 fieldNum;
        FIT_UINT8 size;
        FIT_UINT8 devIndex;
    };

    struct FitLocalDef {
        bool valid = false;
        FIT_UINT16 globalMsgNum = 0;
        bool littleEndian = true;
        std::array<FitFieldDef, 32> fields{};
        size_t fieldCount = 0;
        std::array<FitDevFieldDef, 16> devFields{};
        size_t devFieldCount = 0;
    };

    std::array<FitLocalDef, 16> localDefs{};
    uint16_t lastIndex = 0;
    uint16_t sessionCount = 0;
    uint32_t lastSteps = 0;

    std::array<FIT_UINT8, 256> fieldBuffer{};
    size_t pos = dataStart;
    fp->seek(pos);

    while (pos < dataEnd) {
        FIT_UINT8 headerByte = 0;
        fp->read(reinterpret_cast<char*>(&headerByte), sizeof(FIT_UINT8), br);
        if (br != sizeof(FIT_UINT8)) {
            break;
        }
        pos += 1;

        if (headerByte & 0x40) {
            const FIT_UINT8 localMsg = headerByte & 0x0F;
            FitLocalDef def{};
            FIT_UINT8 reserved = 0;
            FIT_UINT8 arch = 0;
            FIT_UINT8 numFields = 0;
            fp->read(reinterpret_cast<char*>(&reserved), sizeof(FIT_UINT8), br);
            fp->read(reinterpret_cast<char*>(&arch), sizeof(FIT_UINT8), br);
            pos += 2;
            def.littleEndian = (arch == 0);

            FIT_UINT16 globalMsg = 0;
            fp->read(reinterpret_cast<char*>(&globalMsg), sizeof(FIT_UINT16), br);
            pos += 2;
            def.globalMsgNum = def.littleEndian ? globalMsg : static_cast<FIT_UINT16>((globalMsg >> 8) | (globalMsg << 8));

            fp->read(reinterpret_cast<char*>(&numFields), sizeof(FIT_UINT8), br);
            pos += 1;
            def.fieldCount = std::min<size_t>(numFields, def.fields.size());

            for (size_t i = 0; i < numFields; ++i) {
                FitFieldDef field{};
                fp->read(reinterpret_cast<char*>(&field.fieldNum), sizeof(FIT_UINT8), br);
                fp->read(reinterpret_cast<char*>(&field.size), sizeof(FIT_UINT8), br);
                fp->read(reinterpret_cast<char*>(&field.baseType), sizeof(FIT_UINT8), br);
                pos += 3;
                if (i < def.fields.size()) {
                    def.fields[i] = field;
                }
            }

            if (headerByte & 0x20) {
                FIT_UINT8 numDevFields = 0;
                fp->read(reinterpret_cast<char*>(&numDevFields), sizeof(FIT_UINT8), br);
                pos += 1;
                def.devFieldCount = std::min<size_t>(numDevFields, def.devFields.size());
                for (size_t i = 0; i < numDevFields; ++i) {
                    FitDevFieldDef devField{};
                    fp->read(reinterpret_cast<char*>(&devField.fieldNum), sizeof(FIT_UINT8), br);
                    fp->read(reinterpret_cast<char*>(&devField.size), sizeof(FIT_UINT8), br);
                    fp->read(reinterpret_cast<char*>(&devField.devIndex), sizeof(FIT_UINT8), br);
                    pos += 3;
                    if (i < def.devFields.size()) {
                        def.devFields[i] = devField;
                    }
                }
            }

            def.valid = true;
            localDefs[localMsg] = def;
            continue;
        }

        FIT_UINT8 localMsg = 0;
        if (headerByte & 0x80) {
            localMsg = (headerByte >> 5) & 0x03;
        } else {
            localMsg = headerByte & 0x0F;
        }

        if (localMsg >= localDefs.size() || !localDefs[localMsg].valid) {
            break;
        }

        FitLocalDef& def = localDefs[localMsg];
        if (def.globalMsgNum == FIT_MESG_SESSION) {
            sessionCount++;
        }

        for (size_t i = 0; i < def.fieldCount; ++i) {
            const FitFieldDef& field = def.fields[i];
            if (field.size == 0) {
                continue;
            }
            size_t readSize = std::min<size_t>(field.size, fieldBuffer.size());
            fp->read(reinterpret_cast<char*>(fieldBuffer.data()), readSize, br);
            pos += readSize;
            if (field.size > readSize) {
                fp->seek(pos + (field.size - readSize));
                pos += field.size - readSize;
            }

            if (def.globalMsgNum == FIT_MESG_SESSION && field.fieldNum == FIT_SESSION_FIELD_NUM_MESSAGE_INDEX) {
                uint16_t value = 0;
                if (field.size == 1) {
                    value = fieldBuffer[0];
                } else if (field.size >= 2) {
                    if (def.littleEndian) {
                        value = static_cast<uint16_t>(fieldBuffer[0]) |
                               (static_cast<uint16_t>(fieldBuffer[1]) << 8);
                    } else {
                        value = static_cast<uint16_t>(fieldBuffer[1]) |
                               (static_cast<uint16_t>(fieldBuffer[0]) << 8);
                    }
                }
                if (value >= lastIndex) {
                    lastIndex = value;
                }
            }
        }

        for (size_t i = 0; i < def.devFieldCount; ++i) {
            const FitDevFieldDef& devField = def.devFields[i];
            if (devField.size == 0) {
                continue;
            }
            size_t readSize = std::min<size_t>(devField.size, fieldBuffer.size());
            fp->read(reinterpret_cast<char*>(fieldBuffer.data()), readSize, br);
            pos += readSize;
            if (def.globalMsgNum == FIT_MESG_RECORD && devField.devIndex == 0 && devField.fieldNum == 0 &&
                devField.size >= sizeof(uint32_t)) {
                std::array<FIT_UINT8, sizeof(uint32_t)> stepsBytes{};
                std::memcpy(stepsBytes.data(), fieldBuffer.data(), sizeof(uint32_t));
                if (!def.littleEndian) {
                    std::reverse(stepsBytes.begin(), stepsBytes.end());
                }
                uint32_t value = 0;
                std::memcpy(&value, stepsBytes.data(), sizeof(uint32_t));
                lastSteps = value;
            }
            if (devField.size > readSize) {
                fp->seek(pos + (devField.size - readSize));
                pos += devField.size - readSize;
            }
        }
    }

    if (sessionCount > 0) {
        mSessionIndex = sessionCount;
        if (!mStepsLoaded) {
            mTotalSteps = lastSteps;
            mStepsLoaded = true;
        }
    } else {
        mSessionIndex = 0;
    }
    mSessionIndexInitialized = true;
    LOG_DEBUG("loadSessionIndex: lastIndex=%u, sessionCount=%u, stepsLoaded=%d, mSessionIndex=%u\n",
              static_cast<unsigned>(lastIndex), static_cast<unsigned>(sessionCount), mStepsLoaded, static_cast<unsigned>(mSessionIndex));
}

void Service::appendPendingRecords(SDK::Interface::IFile* fp) {
    if (mPendingRecords.empty()) {
        return;
    }

    for (const auto& rec : mPendingRecords) {
        FIT_RECORD_MESG record_mesg{};
        record_mesg.timestamp = unixToFitTimestamp(rec.timestamp);
        record_mesg.heart_rate = rec.heartRate;
        mFitRecord.writeMessage(&record_mesg, fp);

        uint32_t steps = rec.steps;
        mFitRecord.writeFieldMessage(0, &steps, fp);
    }

    mPendingRecords.clear();
}

void Service::writeFitSessionSummary(SDK::Interface::IFile* fp, std::time_t timestamp) {
    FIT_EVENT_MESG stop_event{};
    stop_event.timestamp = unixToFitTimestamp(timestamp);
    stop_event.event = FIT_EVENT_TIMER;
    stop_event.event_type = FIT_EVENT_TYPE_STOP;
    mFitEvent.writeMessage(&stop_event, fp);

    FIT_SESSION_MESG session_mesg{};
    if (!mSessionIndexInitialized) {
        mSessionIndex = 0;
        mSessionIndexInitialized = true;
    }
    session_mesg.message_index = mSessionIndex;
    session_mesg.sport = FIT_SPORT_GENERIC;
    session_mesg.sub_sport = FIT_SUB_SPORT_GENERIC;
    session_mesg.timestamp = unixToFitTimestamp(timestamp);
    session_mesg.start_time = unixToFitTimestamp(mDayStart);
    session_mesg.total_elapsed_time = static_cast<FIT_UINT32>((timestamp - mDayStart) * 1000);
    session_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mDayStart) * 1000);
    mFitSession.writeMessage(&session_mesg, fp);

    FIT_ACTIVITY_MESG activity_mesg{};
    activity_mesg.timestamp = unixToFitTimestamp(timestamp);
    activity_mesg.local_timestamp = unixToFitTimestamp(epochToLocal(timestamp));
    activity_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mDayStart) * 1000);
    mSessionIndex++;
    activity_mesg.num_sessions = mSessionIndex;
    mFitActivity.writeMessage(&activity_mesg, fp);

    mSessionOpen = false;
}

void Service::saveFit(bool force, bool finalizeDay) {
    std::time_t now = std::time(nullptr);

    LOG_DEBUG("saveFit start (force=%d, finalizeDay=%d)\n", force, finalizeDay);

    auto file = mKernel.fs.file(mFitPath);
    if (!file) {
        LOG_ERROR("saveFit failed: can't access FIT path %s\n", mFitPath);
        return;
    }

    bool isNewFile = !file->exist();
    LOG_DEBUG("saveFit opening file %s (new=%d)\n", mFitPath, isNewFile);
    if (!file->open(true, isNewFile)) {
        LOG_ERROR("Failed to open FIT file %s\n", mFitPath);
        return;
    }

    if (!isNewFile) {
        size_t fileSize = file->size();
        LOG_DEBUG("saveFit existing file size=%zu\n", fileSize);
        if (!mSessionIndexInitialized || !mStepsLoaded) {
            loadSessionIndex(file.get());
            mSessionIndexInitialized = true;
            mStepsLoaded = true;
        }
        if (fileSize >= (FIT_FILE_HDR_SIZE + sizeof(FIT_UINT16))) {
            file->truncate(fileSize - sizeof(FIT_UINT16));
            LOG_DEBUG("saveFit truncated CRC, new size=%zu\n", file->size());
        }

        if (file->size() > FIT_FILE_HDR_SIZE) {
            mFitFileInitialized = true;
        }

        file->seek(file->size());
    }

    if (isNewFile || !mFitFileInitialized) {
        LOG_DEBUG("saveFit writing FIT definitions\n");
        writeFitDefinitions(file.get(), now);
        mFitFileInitialized = true;
        mSessionOpen = true;
        if (mDayStart == 0) {
            mDayStart = now;
        }
        if (!mSessionIndexInitialized) {
            mSessionIndex = 0;
            mSessionIndexInitialized = true;
        }
    } else if (!mSessionOpen && !mPendingRecords.empty()) {
        startNewSession(file.get(), now);
    }

    LOG_DEBUG("saveFit appending %zu pending records\n", mPendingRecords.size());
    size_t sizeBeforeAppend = file->size();
    LOG_DEBUG("saveFit size before append=%zu\n", sizeBeforeAppend);
    appendPendingRecords(file.get());
    size_t sizeAfterAppend = file->size();
    LOG_DEBUG("saveFit size after append=%zu\n", sizeAfterAppend);

    if (finalizeDay && mSessionOpen) {
        size_t sizeBeforeSummary = file->size();
        LOG_DEBUG("saveFit size before summary=%zu\n", sizeBeforeSummary);
        LOG_DEBUG("saveFit writing session summary\n");
        writeFitSessionSummary(file.get(), now);
        size_t sizeAfterSummary = file->size();
        LOG_DEBUG("saveFit size after summary=%zu\n", sizeAfterSummary);
    }

    file->flush();
    writeFileHeader(file.get());
    writeCRC(file.get());
    file->close();

    mLastSaveTime = now;
    LOG_INFO("saveFit complete: %s\n", mFitPath);
}