#include "Service.hpp"

#include <ctime>
#include <algorithm>

#include "SDK/Tools/FirmwareVersion.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Timer/Timer.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
    , mGuiStarted(false)
    , mGuiSender(mKernel)
    , mSensorHr(SDK::Sensor::Type::HEART_RATE, skSamplePeriod, skSampleLatency)
    , mHr(0.0f)
    , mHrTl(0.0f)
    , mActivityWriter(mKernel, "Activity")
{}

Service::~Service()
{
    mSensorHr.disconnect();
}

void Service::run()
{
    LOG_INFO("Started\n");

    mSensorHr.connect();

    ActivityWriter::AppInfo info{};
    info.timestamp  = std::time(nullptr);
    info.appVersion = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID      = DEV_ID;
    info.appID      = APP_ID;
    mActivityWriter.start(info);

    time_t startTime    = info.timestamp;
    time_t processedUtc = startTime;  // skip record at the same second as START event

    float    hrAvgSum   = 0.0f;
    uint32_t hrAvgCount = 0;
    float    hrMax      = 0.0f;

    SDK::Timer guiInitTimeout(TIMER_SECONDS(5));
    guiInitTimeout.start();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 500)) {
            // Command handling
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mSensorHr.disconnect();
                    // We must release message because this is the last event.
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    onStartGUI();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    onStopGUI();
                    break;

                // Sensors messages
                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                    handleSensorsData(event->handle, batch);
                } break;

                default:
                    break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        if (mGuiStarted) {
            // Save record to the FIT file every second
            time_t utc = time(nullptr);
            if (processedUtc != utc) {
                processedUtc = utc;

                ActivityWriter::RecordData fitRecord{};
                fitRecord.timestamp  = utc;
                fitRecord.heartRate  = static_cast<uint8_t>(mHr);
                fitRecord.trustLevel = static_cast<uint8_t>(mHrTl);
                mActivityWriter.addRecord(fitRecord);

                if (mHr > 1.0f) {
                    hrAvgSum += mHr;
                    ++hrAvgCount;
                    hrMax = std::max(hrMax, mHr);
                }
            }
        } else {
            // Just wait some time to see if GUI starts
            if (guiInitTimeout.expired()) {
                LOG_INFO("No activities, exiting service\n");
                break;
            }
        }
    }

    time_t utc = time(nullptr);

    // Save lap to the FIT file
    ActivityWriter::LapData fitLap{};
    fitLap.timestamp = utc;
    fitLap.timeStart = startTime;
    fitLap.duration  = utc - startTime;
    fitLap.elapsed   = utc - startTime;
    fitLap.hrAvg     = static_cast<uint8_t>(hrAvgCount > 0 ? hrAvgSum / hrAvgCount : 0);
    fitLap.hrMax     = static_cast<uint8_t>(hrMax);
    mActivityWriter.addLap(fitLap);

    // Save session and close FIT file
    ActivityWriter::TrackData fitTrack{};
    fitTrack.timestamp = utc;
    fitTrack.timeStart = startTime;
    fitTrack.duration  = utc - startTime;
    fitTrack.elapsed   = utc - startTime;
    fitTrack.hrAvg     = static_cast<uint8_t>(hrAvgCount > 0 ? hrAvgSum / hrAvgCount : 0);
    fitTrack.hrMax     = static_cast<uint8_t>(hrMax);
    mActivityWriter.stop(fitTrack);

    mSensorHr.disconnect();

    LOG_INFO("Stopped\n");
}

void Service::onStartGUI()
{
    mGuiStarted = true;
    mGuiSender.heartRate(0.0f, 0.0f);
}

void Service::onStopGUI()
{
    mGuiStarted = false;
}

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorHr.matchesDriver(handle)) {
        if (mGuiStarted) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHr   = parser.getBpm();
                mHrTl = parser.getTrustLevel();

                mGuiSender.heartRate(mHr, mHrTl);
            }
        }
    }
}
