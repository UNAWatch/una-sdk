#include "Service.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"
#include <array>
#include <memory>
#include <cstring>
#include <algorithm>

#define LOG_MODULE_PRX "Service"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserTouch.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"



#include "icon_60x60.h"
#include "icon_30x30.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel),
      mName("Strain Score"),
      mGlanceUI(),
      mGlanceTitle(),
      mGlanceValue(),
      mSensorHR(SDK::Sensor::Type::HEART_RATE),
      mSensorActivity(SDK::Sensor::Type::ACTIVITY),
       mSensorTouch(SDK::Sensor::Type::TOUCH_DETECT),
       mActivityWriter(kernel, "Activities")
{}

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
                LOG_INFO("GLANCE is now running\n");
                mGlanceActive = true;
                break;

            case SDK::MessageType::EVENT_GLANCE_STOP:
                LOG_INFO("GLANCE has stopped\n");
                mGlanceActive = false;
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
    if (!mSensorHR.isConnected()) {
        LOG_DEBUG("Connect to HR sensors...\n");
        mSensorHR.connect(samplePeriodMs);
    }
    if (!mSensorActivity.isConnected()) {
        LOG_DEBUG("Connect to Activity sensor...\n");
        mSensorActivity.connect(samplePeriodMs);
    }
    if (!mSensorTouch.isConnected()) {
        LOG_DEBUG("Connect to Touch sensor...\n");
        mSensorTouch.connect(samplePeriodMs);
    }
}

void Service::disconnect() {
    LOG_DEBUG("Disconnect from sensors...\n");
    if (std::strlen(mCurrentDate) > 0) {
        std::time_t now = std::time(nullptr);
        float avg_hr = (mSampleCount > 0) ? (mSumHR / static_cast<float>(mSampleCount)) : 0.0f;
        ActivityWriter::TrackData track;
        track.timeStart = mDayStart;
        track.duration = now - mDayStart;
        track.elapsed = now - mDayStart;
        track.hrAvg = static_cast<uint8_t>(avg_hr);
        track.hrMax = static_cast<uint8_t>(mMaxHR);
        track.totalAscent = 0;
        track.totalDescent = 0;
        mActivityWriter.stop(track);
    }
    if (mSensorHR.isConnected()) {
        mSensorHR.disconnect();
    }
    if (mSensorActivity.isConnected()) {
        mSensorActivity.disconnect();
    }
    if (mSensorTouch.isConnected()) {
        mSensorTouch.disconnect();
    }
}

void Service::onSdlNewData(uint16_t handle, const SDK::Sensor::Data* data, uint16_t count, uint16_t stride) {
    std::time_t now = std::time(nullptr);
    checkDayRollover();

    SDK::Sensor::DataBatch batch(data, count, stride);

    if (mSensorTouch.matchesDriver(handle)) {
        if (count > 0) {
            SDK::SensorDataParser::Touch p(batch[0]);
            if (p.isDataValid()) {
                bool onHand = p.isTouched();
                if (mIsOnHand != onHand) {
                    mIsOnHand = onHand;
                    if (!mIsOnHand) {
                        LOG_INFO("Watch hand off\n");
                    } else {
                        LOG_INFO("Watch is worn on the wrist\n");
                    }
                }
            }
        }
    }

    if (mSensorActivity.matchesDriver(handle)) {
        if (count > 0) {
            SDK::SensorDataParser::Activity p(batch[0]);
            if (p.isDataValid()) {
                mActiveMin = p.getDuration();
                if (mIsOnHand && mSampleCount > 0) {
                    LOG_DEBUG("strain: %f active_min: %u\n", static_cast<double>(mTotalStrain),
                              static_cast<unsigned>(mActiveMin));
                    ActivityWriter::RecordData rec;
                    rec.timestamp = now;
                    rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
                    rec.heartRate = static_cast<uint8_t>(std::min<uint16_t>(mLastHr, 255));
                    rec.set(ActivityWriter::RecordData::Field::STRAIN);
                    rec.strain = mTotalStrain;
                    rec.set(ActivityWriter::RecordData::Field::ACTIVE_MIN);
                    rec.activeMin = mActiveMin;
                    mActivityWriter.addRecord(rec);
                }
            }
        }
    }

    if (mSensorHR.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::HeartRate p(batch[i]);
            if (!p.isDataValid() || !mIsOnHand) continue;
            float hrRaw = p.getBpm();
            LOG_DEBUG("HR: %f\n", hrRaw);
            uint16_t hr = static_cast<uint16_t>(hrRaw);
            if (hr >= 50 && hr <= 255) {
                float norm = (static_cast<float>(hr) - 60.0f) / 120.0f;
                float delta = std::max(0.0f, norm) * 0.75f;
                mTotalStrain += delta;
                mSumHR += static_cast<float>(hr);
                mMaxHR = std::max(mMaxHR, hr);
                mSampleCount++;
                mLastHr = hr;
            }
        }
    }


}

void Service::onGlanceTick() {
    mGlanceValue.print("%.1f", mTotalStrain);

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
            if (gc->maxControls >= 3) {
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
        .setText("Strain Score")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    mGlanceValue = mGlanceUI.createText();
    mGlanceValue.pos({80, 28}, {80, 34})
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
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
            // finalize previous day
            float avg_hr = (mSampleCount > 0) ? (mSumHR / static_cast<float>(mSampleCount)) : 0.0f;
            ActivityWriter::TrackData track;
            track.timeStart = mDayStart;
            track.duration = t_now - mDayStart;
            track.elapsed = t_now - mDayStart;
            track.hrAvg = static_cast<uint8_t>(avg_hr);
            track.hrMax = static_cast<uint8_t>(mMaxHR);
            track.totalAscent = 0;
            track.totalDescent = 0;
            mActivityWriter.stop(track);
        }
        std::strncpy(mCurrentDate, now_date, 10);
        mCurrentDate[10] = '\0';

        // start new day
        ActivityWriter::AppInfo info;
        info.timestamp = t_now;
        info.appVersion = 1; // hardcode
        info.devID = "UNA";
        info.appID = "STRAIN";
        mActivityWriter.start(info);

        mTotalStrain = 0.0f;
        mSumHR = 0.0f;
        mMaxHR = 0;
        mActiveMin = 0;
        mSampleCount = 0;
        mLastHr = 0;
        mDayStart = t_now;

        LOG_INFO("New day: %s\\n", mCurrentDate);
    }
}
