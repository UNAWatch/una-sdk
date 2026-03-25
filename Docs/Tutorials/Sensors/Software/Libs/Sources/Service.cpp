#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAltimeter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometer.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include <cmath>

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
    , mSensorBattery(SDK::Sensor::Type::BATTERY_LEVEL, 0, 0)
    , mSensorAccelerometerRaw(SDK::Sensor::Type::ACCELEROMETER_RAW, 0, 0)
    , mSensorGyroscope(SDK::Sensor::Type::GYROSCOPE, 0, 0)
    , mSensorGyroscopeRaw(SDK::Sensor::Type::GYROSCOPE_RAW, 0, 0)
    , mSensorHeartBeat(SDK::Sensor::Type::HEART_BEAT, 0, 0)
    , mSensorHeartRateMetrics(SDK::Sensor::Type::HEART_RATE_METRICS, 0, 0)
    , mSensorStepDetector(SDK::Sensor::Type::STEP_DETECTOR, 0, 0)
    , mSensorAmbientTemperature(SDK::Sensor::Type::AMBIENT_TEMPERATURE, 0, 0)
    , mSensorPressure(SDK::Sensor::Type::PRESSURE, 0, 0)
    , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION, 0, 0)
    , mSensorMotionDetect(SDK::Sensor::Type::MOTION_DETECT, 0, 0)
    , mSensorActivityRecognition(SDK::Sensor::Type::ACTIVITY_RECOGNITION, 0, 0)
    , mSensorGestureRecognition(SDK::Sensor::Type::GESTURE_RECOGNITION, 0, 0)
    , mSensorActivity(SDK::Sensor::Type::ACTIVITY, 0, 0)
    , mSensorPPG(SDK::Sensor::Type::PPG, 0, 0)
    , mSensorECG(SDK::Sensor::Type::ECG, 0, 0)
    , mSensorGPSSpeed(SDK::Sensor::Type::GPS_SPEED, 0, 0)
    , mSensorGPSDistance(SDK::Sensor::Type::GPS_DISTANCE, 0, 0)
    , mSensorBatteryCharging(SDK::Sensor::Type::BATTERY_CHARGING, 0, 0)
    , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, 0, 0)
    , mSensorFusion(SDK::Sensor::Type::FUSION, 0, 0)
    , mSensorFusionRaw(SDK::Sensor::Type::FUSION_RAW, 0, 0)
    , mSensorTouchDetect(SDK::Sensor::Type::TOUCH_DETECT, 0, 0)
    , mHR(0)
    , mHRTL(0)
    , mServiceCpuTimeMs(0)
    , mGuiCpuTimeMs(0)
    , mActiveTimeMs(0)
    , mTxMessages(0)
    , mRxMessages(0)
    , mTxBytes(0)
    , mRxBytes(0)
    , mLastStatsTimeMs(0)
    , mLastAccTimeMs(0)
    , mLastMagTimeMs(0)
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
    mSensorBattery.connect();
    mSensorAccelerometerRaw.connect();
    mSensorGyroscope.connect();
    mSensorGyroscopeRaw.connect();
    mSensorHeartBeat.connect();
    mSensorHeartRateMetrics.connect();
    mSensorStepDetector.connect();
    mSensorAmbientTemperature.connect();
    mSensorPressure.connect();
    mSensorWristMotion.connect();
    mSensorMotionDetect.connect();
    mSensorActivityRecognition.connect();
    mSensorGestureRecognition.connect();
    mSensorActivity.connect();
    mSensorPPG.connect();
    mSensorECG.connect();
    mSensorGPSSpeed.connect();
    mSensorGPSDistance.connect();
    mSensorBatteryCharging.connect();
    mSensorBatteryMetrics.connect();
    mSensorFusion.connect();
    mSensorFusionRaw.connect();
    mSensorTouchDetect.connect();
    LOG_INFO("Note: No BLE calibration at the moment. BLE calibration is required for proper sensor operation, especially for HR.\n");


    mLastStatsTimeMs = mKernel.sys.getTimeMs();

    uint32_t startTimeMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            uint32_t processStart = mKernel.sys.getTimeMs();
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
                    mSensorBattery.disconnect();
                    mSensorAccelerometerRaw.disconnect();
                    mSensorGyroscope.disconnect();
                    mSensorGyroscopeRaw.disconnect();
                    mSensorHeartBeat.disconnect();
                    mSensorHeartRateMetrics.disconnect();
                    mSensorStepDetector.disconnect();
                    mSensorAmbientTemperature.disconnect();
                    mSensorPressure.disconnect();
                    mSensorWristMotion.disconnect();
                    mSensorMotionDetect.disconnect();
                    mSensorActivityRecognition.disconnect();
                    mSensorGestureRecognition.disconnect();
                    mSensorActivity.disconnect();
                    mSensorPPG.disconnect();
                    mSensorECG.disconnect();
                    mSensorGPSSpeed.disconnect();
                    mSensorGPSDistance.disconnect();
                    mSensorBatteryCharging.disconnect();
                    mSensorBatteryMetrics.disconnect();
                    mSensorFusion.disconnect();
                    mSensorFusionRaw.disconnect();
                    mSensorTouchDetect.disconnect();
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
                    mRxBytes += event->count * event->stride;
                    onSdlNewData(event->handle, data);
                } break;

                default:
                    break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
            uint32_t processEnd = mKernel.sys.getTimeMs();
            mActiveTimeMs += (processEnd - processStart);
        }

        if (mGUIStarted) {
            // Update CPU time and message rates every second
            uint32_t currentTimeMs = mKernel.sys.getTimeMs();
            if (currentTimeMs - mLastStatsTimeMs >= 1000) {
                // Calculate service CPU time (active processing time, excluding wait time)
                mServiceCpuTimeMs = mActiveTimeMs;
                // GUI CPU time would need to be tracked separately, for now set to 0
                mGuiCpuTimeMs = 0;

                // Log stats
                // Calculate simplistic CPU % (ms per sec /10)
                float serviceCpuPct = static_cast<float>(mServiceCpuTimeMs) / 10.0f;
                float guiCpuPct = static_cast<float>(mGuiCpuTimeMs) / 10.0f;
                mSender.updateStats(serviceCpuPct, guiCpuPct,
                                    static_cast<float>(mTxMessages),
                                    static_cast<float>(mRxMessages),
                                    static_cast<float>(mTxBytes),
                                    static_cast<float>(mRxBytes));
                LOG_INFO("Stats sent: SCPU%.1f%% GCPU%.1f%% TX:%.0f msg/s (%.0f B/s) RX:%.0f msg/s (%.0f B/s)\n",
                         serviceCpuPct, guiCpuPct,
                         static_cast<float>(mTxMessages), static_cast<float>(mTxBytes),
                         static_cast<float>(mRxMessages), static_cast<float>(mRxBytes));

                // Send RTC time (seconds since boot)
                uint32_t rtcTime = static_cast<uint32_t>(mKernel.sys.getTimeMs() / 1000ULL);
                mSender.updateRtc(rtcTime);

                // Reset counters
                mTxMessages = 0;
                mRxMessages = 0;
                mTxBytes = 0;
                mRxBytes = 0;
                mActiveTimeMs = 0;
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
    // mSensorAccelerometer.disconnect();
    mSensorStepCounter.disconnect();
    mSensorFloorCounter.disconnect();
    mSensorMagneticField.disconnect();
    mSensorBattery.disconnect();
    mSensorAccelerometerRaw.disconnect();
    mSensorGyroscope.disconnect();
    mSensorGyroscopeRaw.disconnect();
    mSensorHeartBeat.disconnect();
    mSensorHeartRateMetrics.disconnect();
    mSensorStepDetector.disconnect();
    mSensorAmbientTemperature.disconnect();
    mSensorPressure.disconnect();
    mSensorWristMotion.disconnect();
    mSensorMotionDetect.disconnect();
    mSensorActivityRecognition.disconnect();
    mSensorGestureRecognition.disconnect();
    mSensorActivity.disconnect();
    mSensorPPG.disconnect();
    mSensorECG.disconnect();
    mSensorGPSSpeed.disconnect();
    mSensorGPSDistance.disconnect();
    mSensorBatteryCharging.disconnect();
    mSensorBatteryMetrics.disconnect();
    mSensorFusion.disconnect();
    mSensorFusionRaw.disconnect();
    mSensorTouchDetect.disconnect();

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
        if (mSensorHR.matchesDriver(handle)) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHR   = parser.getBpm();
                mHRTL = parser.getTrustLevel();
                // LOG_DEBUG("HR: %.0f BPM\n", mHR);
                mTxMessages++;
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
                // LOG_DEBUG("GPS: %.6f, %.6f, %.1f\n", latitude, longitude, altitude);
                mTxMessages++;
                mSender.updateLocation(timestamp, latitude, longitude, altitude);
                mTxBytes += sizeof(CustomMessage::LocationValues);
            }
        } else if (mSensorAltimeter.matchesDriver(handle)) {
            SDK::SensorDataParser::Altimeter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                float elevation = parser.getAltitude();
                // LOG_DEBUG("Elevation: %.1f m\n", elevation);
                mTxMessages++;
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
                uint64_t nowMs = mKernel.sys.getTimeMs();
                if (nowMs - mLastAccTimeMs >= 100) {
                    // LOG_DEBUG("Acc: %.2f, %.2f, %.2f, now: %u, last: %u, timestamp: %llu\n", x, y, z, nowMs, mLastAccTimeMs, timestamp);
                    mLastAccTimeMs = nowMs;
                    mTxMessages++;
                    mSender.updateAccelerometer(timestamp, x, y, z);
                    mTxBytes += sizeof(CustomMessage::AccelerometerValues);
                }
            }
        } else if (mSensorStepCounter.matchesDriver(handle)) {
            SDK::SensorDataParser::StepCounter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                uint32_t steps = parser.getStepCount();
                // LOG_DEBUG("Steps: %u\n", steps);
                mTxMessages++;
                mSender.updateStepCounter(timestamp, steps);
                mTxBytes += sizeof(CustomMessage::StepCounterValues);
            }
        } else if (mSensorFloorCounter.matchesDriver(handle)) {
            SDK::SensorDataParser::FloorCounter parser(data[0]);
            if (parser.isDataValid()) {
                uint64_t timestamp = parser.getTimestamp();
                uint32_t floors = static_cast<uint32_t>(parser.getFloorsUp());
                // LOG_DEBUG("Floors: %u\n", floors);
                mTxMessages++;
                mSender.updateFloors(timestamp, floors);
                mTxBytes += sizeof(CustomMessage::FloorsValues);
            }
        } else if (mSensorMagneticField.matchesDriver(handle)) {
            SDK::Sensor::DataView view(data[0]);
            float x = view.f[0];
            float y = view.f[1];
            float heading = atan2f(y, x) * (180.0f / M_PI);
            if (heading < 0.0f) heading += 360.0f;
            auto nowMs = mKernel.sys.getTimeMs();
            if (nowMs - mLastMagTimeMs >= 100) {
                // LOG_DEBUG("Compass: %.1f deg (X:%.2f Y:%.2f)\n", heading, x, y);
                mTxMessages++;
                mSender.updateCompass(nowMs, heading);
                mTxBytes += sizeof(CustomMessage::CompassValues);
                mLastMagTimeMs = nowMs;
            }
        } else if (mSensorBattery.matchesDriver(handle)) {
            SDK::SensorDataParser::BatteryLevel parser(data[0]);
            if (parser.isDataValid()) {
                float level = parser.getCharge();
                // LOG_DEBUG("Battery: %.1f%%\n", level);
                mTxMessages++;
                mSender.updateBattery(level);
                mTxBytes += sizeof(CustomMessage::BatteryValues);
            }
        } else if (mSensorAccelerometerRaw.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: ACCELEROMETER_RAW, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorGyroscope.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: GYROSCOPE, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorGyroscopeRaw.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: GYROSCOPE_RAW, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorHeartBeat.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: HEART_BEAT, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorHeartRateMetrics.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: HEART_RATE_METRICS, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorStepDetector.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: STEP_DETECTOR, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorAmbientTemperature.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: AMBIENT_TEMPERATURE, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorPressure.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: PRESSURE, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorWristMotion.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: WRIST_MOTION, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorMotionDetect.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: MOTION_DETECT, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorActivityRecognition.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: ACTIVITY_RECOGNITION, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorGestureRecognition.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: GESTURE_RECOGNITION, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorActivity.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: ACTIVITY, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorPPG.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: PPG, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorECG.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: ECG, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorGPSSpeed.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: GPS_SPEED, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorGPSDistance.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: GPS_DISTANCE, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorBatteryCharging.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: BATTERY_CHARGING, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorBatteryMetrics.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: BATTERY_METRICS, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorFusion.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: FUSION, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorFusionRaw.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: FUSION_RAW, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
        } else if (mSensorTouchDetect.matchesDriver(handle)) {
            LOG_DEBUG("Sensor: TOUCH_DETECT, Binary: ");
            SDK::Sensor::DataView view(data[0]);
            for (size_t i = 0; i < data.stride; ++i) {
                LOG_DEBUG("%02x ", view.u8[i]);
            }
            LOG_DEBUG("\n");
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
