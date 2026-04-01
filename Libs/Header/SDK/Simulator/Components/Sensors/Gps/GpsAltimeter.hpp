/**
 ******************************************************************************
 * @file    GpsAltimeter.hpp
 * @date    10-November-2025
 * @author  Vlad
 * @brief   Sensor for the GPS Altimeter
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GPS_ALTIMETER_HPP
#define __GPS_ALTIMETER_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>
#include <cstdint>


using namespace Interface;

namespace Sensor
{
    class GpsAltimeter : public Interface::ISensor,
                         public Sensor::ISensorDriverCtrl
    {
    public:
        GpsAltimeter();
        
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
        float             mData;
        OS::Mutex         mDataMutex;
        Interface::IGps&  mGps;
    }; /* class GpsAltimeter */

} /* namespace Sensor */

#endif /* __GPS_ALTIMETER_HPP */
