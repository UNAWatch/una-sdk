#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAltimeter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometer.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"

#include "Service.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSender(mKernel)
    , mGUIStarted(false)
    , mSensorHR(SDK::Sensor::Type::HEART_RATE, 0, 0)
    , mSensorGPS(SDK::Sensor::Type::GPS_LOCATION, 0, 0)
    , mSensorAltimeter(SDK::Sensor::Type::ALTIMETER, 0, 0)
    , mSensorAccelerometer(SDK::Sensor::Type::ACCELEROMETER, 0, 0)
    , mSensorStepCounter(SDK::Sensor::Type::STEP_COUNTER, 0, 0)
    , mSensorFloorCounter(SDK::Sensor::Type::FLOOR_COUNTER, 0, 0)
    , mSensorMagneticField(SDK::Sensor::Type::MAGNETIC_FIELD, 0, 0)
    , mHR(0)
    , mHRTL(0)
    , mServiceCpuTimeMs(0)
    , mGuiCpuTimeMs(0)
    , mTxMessages(0)
    , mRxMessages(0)
    , mTxBytes(0)
    , mRxBytes(0)
    , mLastStatsTimeMs(0)
{}

void Service::run()
{
    LOG_INFO("thread started\n");

    mSensorHR.connect();
    mSensorGPS.connect();
    mSensorAltimeter.connect();
    mSensorAccelerometer.connect();
    mSensorStepCounter.connect();
    mSensorFloorCounter.connect();
    mSensorMagneticField.connect();

    mLastStatsTimeMs = mKernel.sys.getTimeMs();

    uint32_t startTimeMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            // Track received messages
            mRxMessages++;
            // mRxBytes += msg->getSize(); // no getSize method

            // Command handling
            switch (msg->getType()) {
                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mSensorHR.disconnect();
                    mSensorGPS.disconnect();
                    mSensorAltimeter.disconnect();
                    mSensorAccelerometer.disconnect();
                    mSensorStepCounter.disconnect();
                    mSensorFloorCounter.disconnect();
                    mSensorMagneticField.disconnect();
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
            // Update CPU time and message rates every second
            uint32_t currentTimeMs = mKernel.sys.getTimeMs();
            if (currentTimeMs - mLastStatsTimeMs >= 1000) {
                // Calculate service CPU time (simplified, just elapsed time)
                mServiceCpuTimeMs += (currentTimeMs - mLastStatsTimeMs);
                // GUI CPU time would need to be tracked separately, for now set to 0
                mGuiCpuTimeMs = 0;

                // Log stats
                LOG_INFO("Stats: Service CPU %u ms, GUI CPU %u ms, TX: %u msg/s (%u B/s), RX: %u msg/s (%u B/s)\n",
                         mServiceCpuTimeMs, mGuiCpuTimeMs,
                         mTxMessages, mTxBytes, mRxMessages, mRxBytes);

                // Reset counters
                mTxMessages = 0;
                mRxMessages = 0;
                mTxBytes = 0;
                mRxBytes = 0;
                mLastStatsTimeMs = currentTimeMs;
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

    mSensorHR.disconnect();
    mSensorGPS.disconnect();
    mSensorAltimeter.disconnect();
    mSensorAccelerometer.disconnect();
    mSensorStepCounter.disconnect();
    mSensorFloorCounter.disconnect();
    mSensorMagneticField.disconnect();

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
    if (mGUIStarted) {
        // Track transmitted messages
        mTxMessages++;
        // Note: bytes will be tracked when sending

        if (mSensorHR.matchesDriver(handle)) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHR   = parser.getBpm();
                mHRTL = parser.getTrustLevel();
                mSender.updateHeartRate(mHR, mHRTL);
                mTxBytes += sizeof(CustomMessage::HRValues);
            }
        } else if (mSensorGPS.matchesDriver(handle)) {
            SDK::SensorDataParser::GpsLocation parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                float latitude = parser.getLatitude();
                float longitude = parser.getLongitude();
                float altitude = parser.getAltitude();
                mSender.updateLocation(timestamp, latitude, longitude, altitude);
                mTxBytes += sizeof(CustomMessage::LocationValues);
            }
        } else if (mSensorAltimeter.matchesDriver(handle)) {
            SDK::SensorDataParser::Altimeter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                float elevation = parser.getAltitude();
                mSender.updateElevation(timestamp, elevation);
                mTxBytes += sizeof(CustomMessage::ElevationValues);
            }
        } else if (mSensorAccelerometer.matchesDriver(handle)) {
            SDK::SensorDataParser::Accelerometer parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                float x = parser.getX();
                float y = parser.getY();
                float z = parser.getZ();
                mSender.updateAccelerometer(timestamp, x, y, z);
                mTxBytes += sizeof(CustomMessage::AccelerometerValues);
            }
        } else if (mSensorStepCounter.matchesDriver(handle)) {
            SDK::SensorDataParser::StepCounter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                uint32_t steps = parser.getStepCount();
                mSender.updateStepCounter(timestamp, steps);
                mTxBytes += sizeof(CustomMessage::StepCounterValues);
            }
        } else if (mSensorFloorCounter.matchesDriver(handle)) {
            SDK::SensorDataParser::FloorCounter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                uint32_t floors = static_cast<uint32_t>(parser.getFloorsUp());
                mSender.updateFloors(timestamp, floors);
                mTxBytes += sizeof(CustomMessage::FloorsValues);
            }
        } else if (mSensorMagneticField.matchesDriver(handle)) {
            // Note: No specific parser for magnetic field, assuming raw data or skip
            // For now, just log that data was received
            LOG_DEBUG("Magnetic field data received\n");
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
