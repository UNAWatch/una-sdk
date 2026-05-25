#ifndef __INSTANCE_SENSOR_LAYER_HPP
#define __INSTANCE_SENSOR_LAYER_HPP

#include "SDK/Simulator/Components/Sensors/SensorPressure.hpp"
#include "SDK/Simulator/Components/Sensors/HeartRate/SensorHeartRateMetrics.hpp"
#include "SDK/Simulator/Components/Sensors/HeartRate/SensorHeartRate.hpp"
#include "SDK/Simulator/Components/Sensors/SensorBatteryLevel.hpp"
#include "SDK/Simulator/Components/Sensors/Gps/GpsSpeed.hpp"
#include "SDK/Simulator/Components/Sensors/Gps/GpsAltimeter.hpp"
#include "SDK/Simulator/Components/Sensors/Gps/GpsLocation.hpp"
#include "SDK/Simulator/Components/Sensors/Gps/GpsDistance.hpp"
#include "SDK/Simulator/Components/Sensors/IMU/ImuWristMotion.hpp"
#include "SDK/Simulator/Components/Sensors/IMU/ImuStepCounter.hpp"
#include "SDK/Simulator/Components/Sensors/IMU/ImuRunningCadence.hpp"

namespace Instance {
    class SensorLayer
    {
    public:
        static SensorLayer& getInstance();

        void init();
        void handlerButtons(uint8_t key);

    private:
        SensorLayer();
        ~SensorLayer() = default;

        Sensor::GpsAltimeter            mSensorGpsAltimer;
        Sensor::GpsLocation             mSensorGpsLocation;
        Sensor::GpsSpeed                mSensorGpsSpeed;
        Sensor::GpsDistance             mSensorGpsDistance;
        Sensor::BatteryLevel            mSensorBatteryLevel;
        Sensor::ImuWristMotion          mSensorImuWrist;
        Sensor::HeartRateMetrics        mSensorHRMetrics;
        Sensor::HeartRate               mSensorHR;
        Sensor::Pressure                mSensorPressure;
        Sensor::ImuStepCounter          mSensorImuStepCounter;
        Sensor::ImuRunningCadence       mSensorImuRunningCadence;

    };
}

#endif
