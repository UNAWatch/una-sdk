/**
 ******************************************************************************
 * @file    ImuSubSensorAccel.hpp
 * @date    14-November-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SubSensor for the accelerometer
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_P_HPP
#define __SENSOR_P_HPP

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IPressure.hpp"

#include <cstdint>
#include <random>

using namespace Interface;

namespace Sensor
{
    class Pressure :    public ISensor,
                        public Sensor::ISensorDriverCtrl
    {
    public:

        Pressure();

        Sensor::Driver& getDriver();
        
        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;
        //void        sdcNewConnection(Sensor::Driver* driver)              override;

        //// ISensor
        void sensorRefresh() override;

    private:
        float computeP0FromAltitude(float p, float altitude);

        static constexpr uint32_t mMinPeriod = 100; // In ms

        Sensor::Driver        mDriver;
        Interface::IGps&      mGps;
        Interface::IPressure& mPressure;
        ::Driver::SwTimer     mTimer;

    };

} /* namespace Sensor */

#endif /* __SENSOR_P_HPP */
