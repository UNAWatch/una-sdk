#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAltimeter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometer.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Timer/Timer.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

#include "Service.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSender(mKernel)
    , mGUIStarted(false)
    , mSensorHR(SDK::Sensor::Type::HEART_RATE, 1000, 1000)
    , mSensorGPS(SDK::Sensor::Type::GPS_LOCATION, 1000, 1000)
    , mSensorAltimeter(SDK::Sensor::Type::ALTIMETER, 1000, 1000)
    , mSensorAccelerometer(SDK::Sensor::Type::ACCELEROMETER, 1000.0f / 50.0f, 100)
    , mSensorStepCounter(SDK::Sensor::Type::STEP_COUNTER, 1000, 1000)
    , mSensorFloorCounter(SDK::Sensor::Type::FLOOR_COUNTER, 1000, 1000)
    , mSensorMagneticField(SDK::Sensor::Type::MAGNETIC_FIELD, 1000, 1000)
    , mSensorBattery(SDK::Sensor::Type::BATTERY_LEVEL, 1000, 1000)
    , mSensorAccelerometerRaw(SDK::Sensor::Type::ACCELEROMETER_RAW, 1000.0f / 50.0f, 100)
    , mSensorGyroscope(SDK::Sensor::Type::GYROSCOPE, 1000.0f / 50.0f, 100)
    , mSensorGyroscopeRaw(SDK::Sensor::Type::GYROSCOPE_RAW, 1000.0f / 50.0f, 100)
    , mSensorHeartBeat(SDK::Sensor::Type::HEART_BEAT, 1000, 1000)
    , mSensorHeartRateMetrics(SDK::Sensor::Type::HEART_RATE_METRICS, 1000, 1000)
    , mSensorStepDetector(SDK::Sensor::Type::STEP_DETECTOR, 1000, 1000)
    , mSensorAmbientTemperature(SDK::Sensor::Type::AMBIENT_TEMPERATURE, 1000, 1000)
    , mSensorPressure(SDK::Sensor::Type::PRESSURE, 1000, 1000)
    , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION, 1000, 1000)
    , mSensorMotionDetect(SDK::Sensor::Type::MOTION_DETECT, 1000, 1000)
    , mSensorActivityRecognition(SDK::Sensor::Type::ACTIVITY_RECOGNITION, 1000, 1000)
    , mSensorGestureRecognition(SDK::Sensor::Type::GESTURE_RECOGNITION, 1000, 1000)
    , mSensorActivity(SDK::Sensor::Type::ACTIVITY, 1000, 1000)
    , mSensorPPG(SDK::Sensor::Type::PPG, 1000, 1000)
    , mSensorECG(SDK::Sensor::Type::ECG, 1000, 1000)
    , mSensorGPSSpeed(SDK::Sensor::Type::GPS_SPEED, 1000, 1000)
    , mSensorGPSDistance(SDK::Sensor::Type::GPS_DISTANCE, 1000, 1000)
    , mSensorBatteryCharging(SDK::Sensor::Type::BATTERY_CHARGING, 1000, 1000)
    , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, 1000, 1000)
    , mSensorFusion(SDK::Sensor::Type::FUSION, 1000.0f / 50.0f, 100)
    , mSensorFusionRaw(SDK::Sensor::Type::FUSION_RAW, 1000.0f / 50.0f, 100)
    , mSensorTouchDetect(SDK::Sensor::Type::TOUCH_DETECT, 1000, 1000)
    , mHR(0)
    , mHRTL(0)
    , mServiceCpuTimeMs(0)
    , mGuiCpuTimeMs(0)
    , mActiveTimeMs(0)
    , mTxMessages(0)
    , mRxMessages(0)
    , mTxBytes(0)
    , mRxBytes(0)
    , mLastAccTimeMs(0)
    , mLastMagTimeMs(0)
    , mSlStatistic()
{
    mSlStatistic.registration(mSensorHR);
    mSlStatistic.registration(mSensorTouchDetect);
    mSlStatistic.registration(mSensorHeartRateMetrics);
    mSlStatistic.registration(mSensorAccelerometer);
    mSlStatistic.registration(mSensorAccelerometerRaw);
    mSlStatistic.registration(mSensorGyroscope);
    mSlStatistic.registration(mSensorGyroscopeRaw);
    mSlStatistic.registration(mSensorFusion);
    mSlStatistic.registration(mSensorFusionRaw);
    mSlStatistic.registration(mSensorPressure);
    mSlStatistic.registration(mSensorAmbientTemperature);
    mSlStatistic.registration(mSensorStepCounter);
    mSlStatistic.registration(mSensorStepDetector);
    mSlStatistic.registration(mSensorFloorCounter);
    mSlStatistic.registration(mSensorAltimeter);
    mSlStatistic.registration(mSensorWristMotion);
    mSlStatistic.registration(mSensorMotionDetect);
    mSlStatistic.registration(mSensorActivityRecognition);
    mSlStatistic.registration(mSensorActivity);
    mSlStatistic.registration(mSensorGPS);
    mSlStatistic.registration(mSensorGPSSpeed);
    mSlStatistic.registration(mSensorGPSDistance);
    mSlStatistic.registration(mSensorBattery);
    mSlStatistic.registration(mSensorBatteryCharging);
    mSlStatistic.registration(mSensorBatteryMetrics);
    mSlStatistic.registration(mSensorPPG);
    mSlStatistic.registration(mSensorECG);
    mSlStatistic.registration(mSensorHeartBeat);
    mSlStatistic.registration(mSensorGestureRecognition);
    mSlStatistic.registration(mSensorMagneticField);
}

void Service::run()
{
    LOG_INFO("thread started\n");

    slConnect();

    SDK::Timer guiTimeout(5000);
    SDK::Timer statisticTimer(1000);

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
                    slDisconnect();
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
            mSlStatistic.refresh(false);

            // Update CPU time and message rates every second
            if (statisticTimer.tick()) {
                // Calculate service CPU time (active processing time, excluding wait time)
                mServiceCpuTimeMs = mActiveTimeMs;
                // GUI CPU time would need to be tracked separately, for now set to 0
                mGuiCpuTimeMs = 0;

                // Log stats
                // Calculate simplistic CPU % (ms per sec /10)
//                float serviceCpuPct = static_cast<float>(mServiceCpuTimeMs) / 10.0f;
//                float guiCpuPct = static_cast<float>(mGuiCpuTimeMs) / 10.0f;
//                mSender.updateStats(serviceCpuPct, guiCpuPct,
//                                    static_cast<float>(mTxMessages),
//                                    static_cast<float>(mRxMessages),
//                                    static_cast<float>(mTxBytes),
//                                    static_cast<float>(mRxBytes));
//                LOG_INFO("Stats: SCPU=%.1f%% GCPU=%.1f%% TX:%.0f msg/s=%.0f B/s RX:%.0f msg/s=%.0f B/s\n",
//                         serviceCpuPct, guiCpuPct,
//                         static_cast<float>(mTxMessages), static_cast<float>(mTxBytes),
//                         static_cast<float>(mRxMessages), static_cast<float>(mRxBytes));

                // Send RTC time (seconds since boot)
                uint32_t rtcTime = static_cast<uint32_t>(mKernel.sys.getTimeMs() / 1000ULL);
                mSender.updateRtc(rtcTime);

                // Reset counters
                mTxMessages   = 0;
                mRxMessages   = 0;
                mTxBytes      = 0;
                mRxBytes      = 0;
                mActiveTimeMs = 0;
            }
        } else {
            // Just wait some time to see if GUI starts
            if (guiTimeout.expired()) {
                LOG_DEBUG("start GUI timeout\n");
                break;
            }
            mKernel.sys.delay(100);
        }
    }

    slDisconnect();

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

static constexpr const char* type2string(SDK::Sensor::Type type) noexcept
{
    switch (type) {
    case SDK::Sensor::Type::ACCELEROMETER:          return "ACCELEROMETER";
    case SDK::Sensor::Type::ACCELEROMETER_RAW:      return "ACCELEROMETER_RAW";
    case SDK::Sensor::Type::GYROSCOPE:              return "GYROSCOPE";
    case SDK::Sensor::Type::GYROSCOPE_RAW:          return "GYROSCOPE_RAW";
    case SDK::Sensor::Type::MAGNETIC_FIELD:         return "MAGNETIC_FIELD";
    case SDK::Sensor::Type::HEART_BEAT:             return "HEART_BEAT";
    case SDK::Sensor::Type::HEART_RATE:             return "HEART_RATE";
    case SDK::Sensor::Type::HEART_RATE_METRICS:     return "HEART_RATE_METRICS";
    case SDK::Sensor::Type::STEP_DETECTOR:          return "STEP_DETECTOR";
    case SDK::Sensor::Type::STEP_COUNTER:           return "STEP_COUNTER";
    case SDK::Sensor::Type::FLOOR_COUNTER:          return "FLOOR_COUNTER";
    case SDK::Sensor::Type::AMBIENT_TEMPERATURE:    return "AMBIENT_TEMPERATURE";
    case SDK::Sensor::Type::PRESSURE:               return "PRESSURE";
    case SDK::Sensor::Type::ALTIMETER:              return "ALTIMETER";
    case SDK::Sensor::Type::WRIST_MOTION:           return "WRIST_MOTION";
    case SDK::Sensor::Type::MOTION_DETECT:          return "MOTION_DETECT";
    case SDK::Sensor::Type::ACTIVITY_RECOGNITION:   return "ACTIVITY_RECOGNITION";
    case SDK::Sensor::Type::GESTURE_RECOGNITION:    return "GESTURE_RECOGNITION";
    case SDK::Sensor::Type::ACTIVITY:               return "ACTIVITY";
    case SDK::Sensor::Type::PPG:                    return "PPG";
    case SDK::Sensor::Type::ECG:                    return "ECG";
    case SDK::Sensor::Type::GPS_LOCATION:           return "GPS_LOCATION";
    case SDK::Sensor::Type::GPS_SPEED:              return "GPS_SPEED";
    case SDK::Sensor::Type::GPS_DISTANCE:           return "GPS_DISTANCE";
    case SDK::Sensor::Type::BATTERY_LEVEL:          return "BATTERY_LEVEL";
    case SDK::Sensor::Type::BATTERY_CHARGING:       return "BATTERY_CHARGING";
    case SDK::Sensor::Type::BATTERY_METRICS:        return "BATTERY_METRICS";
    case SDK::Sensor::Type::FUSION:                 return "FUSION";
    case SDK::Sensor::Type::FUSION_RAW:             return "FUSION_RAW";
    case SDK::Sensor::Type::TOUCH_DETECT:           return "TOUCH_DETECT";
    case SDK::Sensor::Type::UNKNOWN:
    default:                                        return "UNKNOWN";
    }
}

const char* Service::handle2string(uint16_t handle) const
{
    if (mSensorAccelerometer.matchesDriver(handle))        return "ACCELEROMETER";
    if (mSensorAccelerometerRaw.matchesDriver(handle))     return "ACCELEROMETER_RAW";
    if (mSensorGyroscope.matchesDriver(handle))            return "GYROSCOPE";
    if (mSensorGyroscopeRaw.matchesDriver(handle))         return "GYROSCOPE_RAW";
    if (mSensorMagneticField.matchesDriver(handle))        return "MAGNETIC_FIELD";
    if (mSensorHeartBeat.matchesDriver(handle))            return "HEART_BEAT";
    if (mSensorHR.matchesDriver(handle))                   return "HEART_RATE";
    if (mSensorHeartRateMetrics.matchesDriver(handle))     return "HEART_RATE_METRICS";
    if (mSensorStepDetector.matchesDriver(handle))         return "STEP_DETECTOR";
    if (mSensorStepCounter.matchesDriver(handle))          return "STEP_COUNTER";
    if (mSensorFloorCounter.matchesDriver(handle))         return "FLOOR_COUNTER";
    if (mSensorAmbientTemperature.matchesDriver(handle))   return "AMBIENT_TEMPERATURE";
    if (mSensorPressure.matchesDriver(handle))             return "PRESSURE";
    if (mSensorAltimeter.matchesDriver(handle))            return "ALTIMETER";
    if (mSensorWristMotion.matchesDriver(handle))          return "WRIST_MOTION";
    if (mSensorMotionDetect.matchesDriver(handle))         return "MOTION_DETECT";
    if (mSensorActivityRecognition.matchesDriver(handle))  return "ACTIVITY_RECOGNITION";
    if (mSensorGestureRecognition.matchesDriver(handle))   return "GESTURE_RECOGNITION";
    if (mSensorActivity.matchesDriver(handle))             return "ACTIVITY";
    if (mSensorPPG.matchesDriver(handle))                  return "PPG";
    if (mSensorECG.matchesDriver(handle))                  return "ECG";
    if (mSensorGPS.matchesDriver(handle))                  return "GPS_LOCATION";
    if (mSensorGPSSpeed.matchesDriver(handle))             return "GPS_SPEED";
    if (mSensorGPSDistance.matchesDriver(handle))          return "GPS_DISTANCE";
    if (mSensorBattery.matchesDriver(handle))              return "BATTERY_LEVEL";
    if (mSensorBatteryCharging.matchesDriver(handle))      return "BATTERY_CHARGING";
    if (mSensorBatteryMetrics.matchesDriver(handle))       return "BATTERY_METRICS";
    if (mSensorFusion.matchesDriver(handle))               return "FUSION";
    if (mSensorFusionRaw.matchesDriver(handle))            return "FUSION_RAW";
    if (mSensorTouchDetect.matchesDriver(handle))          return "TOUCH_DETECT";

    return "UNKNOWN";
}

void Service::slConnect()
{
    LOG_INFO("sl connect\n");

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
}

void Service::slDisconnect()
{
    LOG_INFO("sl disconnect\n");

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
}

void Service::onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (!mGUIStarted) {
        return;
    }

//    LOG_INFO("sensor=%s\n", handle2string(handle));

    mSlStatistic.newTransaction(handle, data.size());

    return;

    LOG_INFO("sensor=%s\n", handle2string(handle));

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
        std::stringstream ss;
        ss << "Sensor: ACCELEROMETER_RAW, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorGyroscope.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: GYROSCOPE, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorGyroscopeRaw.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: GYROSCOPE_RAW, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorHeartBeat.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: HEART_BEAT, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorHeartRateMetrics.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: HEART_RATE_METRICS, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorStepDetector.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: STEP_DETECTOR, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorAmbientTemperature.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: AMBIENT_TEMPERATURE, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorPressure.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: PRESSURE, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorWristMotion.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: WRIST_MOTION, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorMotionDetect.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: MOTION_DETECT, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorActivityRecognition.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: ACTIVITY_RECOGNITION, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorGestureRecognition.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: GESTURE_RECOGNITION, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorActivity.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: ACTIVITY, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorPPG.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: PPG, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorECG.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: ECG, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorGPSSpeed.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: GPS_SPEED, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorGPSDistance.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: GPS_DISTANCE, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorBatteryCharging.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: BATTERY_CHARGING, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorBatteryMetrics.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: BATTERY_METRICS, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorFusion.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: FUSION, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorFusionRaw.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: FUSION_RAW, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    } else if (mSensorTouchDetect.matchesDriver(handle)) {
        std::stringstream ss;
        ss << "Sensor: TOUCH_DETECT, Binary: ";
        SDK::Sensor::DataView view(data[0]);
        for (size_t i = 0; i < view.getFieldCount(); ++i) {
            uint32_t val = view.u[i];
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[j]) << " ";
            }
        }
        LOG_DEBUG("%s\n", ss.str().c_str());
    }
}
