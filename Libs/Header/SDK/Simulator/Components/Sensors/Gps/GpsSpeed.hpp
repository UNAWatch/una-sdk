/**
 ******************************************************************************
 * @file    GpsSpeed.hpp
 * @date    28-October-2025
 * @author  Vlad
 * @brief   Sensor for the GPS Speed
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GPS_SPEED_HPP
#define __GPS_SPEED_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>
#include <cstdint>

namespace Sensor
{
    class GpsSpeed : public Interface::ISensor,
                              public Sensor::ISensorDriverCtrl
    {
    public:
        GpsSpeed();
        
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
        static constexpr uint32_t mMinPeriod = 1000; // In ms

        Sensor::Driver    mDriver;
        ::Driver::SwTimer mTimer;
        Interface::IGps& mGps;
    }; /* class GpsSpeed */

} /* namespace Sensor */

#endif /* __GPS_SPEED_HPP */
