#define LOG_MODULE_PRX      "SL::Stat"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SLStatistic.hpp"

SLStatistic::Item::Item(SDK::Sensor::Connection& conn)
    : connection(conn)
    , packages(0)
    , samples(0)
{
}

SLStatistic::SLStatistic()
    : mItems()
    , mTimer(TIMER_SECONDS(5))
{
    mItems.reserve(30);
}

void SLStatistic::registration(SDK::Sensor::Connection& connection)
{
    mItems.emplace_back(connection);
}

bool SLStatistic::newTransaction(uint16_t handle, uint32_t samples)
{
    for (auto& i : mItems) {
        if (i.connection.matchesDriver(handle)) {
            i.packages += 1;
            i.samples  += samples;
            return true;
        }
    }

    return false;
}

void SLStatistic::refresh(bool autoClear)
{
    if (mTimer.tick()) {
        show();
        if (autoClear) {
            clear();
        }
    }
}

void SLStatistic::clear()
{
    for (auto& i : mItems) {
        i.packages = 0;
        i.samples  = 0;
    }
}

void SLStatistic::show()
{
    LOG_INFO("Sensor Name          | packages | samples\n");
    LOG_INFO("-----------------------------------------\n");
    for (auto& i : mItems) {
        char packages[15]{};
        if (i.packages) {
            snprintf(packages, sizeof(packages), "%lu", i.packages);
        } else {
            for (uint8_t idx = 0; idx < sizeof(packages) - 1; ++idx) {
                packages[idx] = ' ';
            }
            packages[8] = '\0';
        }

        char samples[15]{};
        if (i.samples) {
            snprintf(samples, sizeof(samples), "%lu", i.samples);
        } else {
            for (uint8_t idx = 0; idx < sizeof(samples) - 1; ++idx) {
                samples[idx] = ' ';
            }
            samples[7] = '\0';
        }

        LOG_INFO("%-20s | %8s | %7s\n", type2string(i.connection.getID()), packages, samples);
    }
}

const char* SLStatistic::type2string(SDK::Sensor::Type type) const
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
