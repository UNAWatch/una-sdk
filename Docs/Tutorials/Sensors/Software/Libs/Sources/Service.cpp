#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"

#include "Service.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSender(mKernel)
    , mGUIStarted(false)
    , mSensorHR(SDK::Sensor::Type::HEART_RATE, 1000, 2000)
    , mHR(0)
    , mHRTL(0)
    , mActivityWriter(mKernel, "Activity")
{}

void Service::run()
{
    LOG_INFO("thread started\n");

    mSensorHR.connect();

    ActivityWriter::AppInfo info{};
    info.timestamp  = std::time(nullptr);
    info.appVersion = ParseVersion(BUILD_VERSION);
    info.devID      = DEV_ID;
    info.appID      = APP_ID;
    mActivityWriter.start(info);

    time_t startTime    = time(nullptr);
    time_t utcTimestamp = 0;

    float    hrAvgSum   = 0;
    uint32_t hrAvgCount = 0;
    float    hrMax      = 0;

    uint32_t startTimeMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            // Command handling
            switch (msg->getType()) {
                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mSensorHR.disconnect();
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
                    SDK::Sensor::DataBatch data(event->data, event->count, event->stride);
                    onSdlNewData(event->handle, data);
                } break;

                default:
                    break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        if (mGUIStarted) {
            // Save record to the FIT file
            time_t utc = time(nullptr);
            if (utcTimestamp != utc) {
                utcTimestamp = utc;

                ActivityWriter::RecordData fitRecord {};
                fitRecord.timestamp  = utc;
                fitRecord.heartRate  = static_cast<uint8_t>(mHR);
                fitRecord.trustLevel = static_cast<uint8_t>(mHRTL);
                mActivityWriter.addRecord(fitRecord);

                if (mHR > 1) {
                    hrAvgSum += mHR;
                    ++hrAvgCount;
                    hrMax = std::max(hrMax, mHR);
                }
            }
        } else {
            // Just wait some time to see if GUI starts
            if (mKernel.sys.getTimeMs() - startTimeMs > 5000) {
                LOG_DEBUG("start GUI timeout\n");
                break;
            }
            mKernel.sys.delay(100);
        }
    }

    time_t utc = time(nullptr);

    // Save lap to the FIT file
    ActivityWriter::LapData fitLap {};
    fitLap.timeStart = utc - startTime;
    fitLap.duration = utc - startTime;
    fitLap.elapsed = utc - startTime;
    fitLap.hrAvg = static_cast<uint8_t>(hrAvgSum / hrAvgCount);
    fitLap.hrMax = static_cast<uint8_t>(hrMax);
    mActivityWriter.addLap(fitLap);

    // Create FIT file

    ActivityWriter::TrackData fitTrack{};
    fitTrack.timeStart = utc;
    fitTrack.duration = utc - startTime;
    fitTrack.elapsed = utc - startTime;
    fitTrack.hrAvg = static_cast<uint8_t>(hrAvgSum / hrAvgCount);
    fitTrack.hrMax = static_cast<uint8_t>(hrMax);

    mActivityWriter.stop(fitTrack);

    mSensorHR.disconnect();

    LOG_INFO("thread stopped\n");
}

void Service::onStartGUI()
{
    LOG_INFO("GUI started\n");
    mGUIStarted = true;
    mSender.updateHeartRate(0.0f, 0.0f);
}

void Service::onStopGUI()
{
    LOG_INFO("GUI stopped\n");
    mGUIStarted = false;
}

void Service::onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorHR.matchesDriver(handle)) {
        if (mGUIStarted) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHR   = parser.getBpm();
                mHRTL = parser.getTrustLevel();

                mSender.updateHeartRate(mHR, mHRTL);
            }
        }
    }
}

uint32_t Service::ParseVersion(const char* str)
{
    if (str == nullptr) {
        return 0;
    }

    typedef union {
        struct {
            uint8_t patch;
            uint8_t minor;
            uint8_t major;
        };
        uint32_t u32;
    } FirmwareVersion_t;

    FirmwareVersion_t v{};

    int major, minor, patch;

    if (sscanf(str, "%d.%d.%d", &major, &minor, &patch) == 3) {
        v.major = static_cast<uint8_t>(major);
        v.minor = static_cast<uint8_t>(minor);
        v.patch = static_cast<uint8_t>(patch);
        return v.u32;
    }

    return 0;
}
