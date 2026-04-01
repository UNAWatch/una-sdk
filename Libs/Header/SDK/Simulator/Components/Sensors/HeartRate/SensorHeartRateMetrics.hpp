/**
 ******************************************************************************
 * @file    SensorHeartRateMetrics.cpp
 * @date    15-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Additional metrics from the heart rate
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_HEART_RATE_METRICS_HPP
#define __SENSOR_HEART_RATE_METRICS_HPP

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IHeatRate.hpp"

#include <cstdint>

using namespace Interface;

namespace Sensor
{
    class HeartRateMetrics : public ISensor,
                             public Sensor::ISensorDriverCtrl
    {
    public:
        HeartRateMetrics();
        
        Sensor::Driver& getDriver();

        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;

        //// ISensor
        void sensorRefresh() override;

    private:
        void publishData();

        static constexpr float mMinPeriod = 500;

        struct Data {
            bool  updated;
            float ahr;
            float rhr;
        };

        Sensor::Driver                  mDriver;
        Interface::IHeartRate&          mpHeatRateSim;
        ::Driver::SwTimer               mUpdateTimer;
        Data                            mData;
    }; /* class HeartRateMetrics */

} /* namespace Sensor */

#endif /* __SENSOR_HEART_RATE_METRICS_HPP */
