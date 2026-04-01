/**
 ******************************************************************************
 * @file    SensorBatteryLevel.hpp
 * @date    23-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor Battery Level
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_BATTERY_LEVEL_HPP
#define __SENSOR_BATTERY_LEVEL_HPP

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IBatteryLevel.hpp"

#include <cstdint>

namespace Sensor
{
    class BatteryLevel : public Interface::ISensor,
                         public Sensor::ISensorDriverCtrl
    {
    public:
        BatteryLevel();
        
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
        void publishData(float level);

        static constexpr uint32_t mMinPeriod = 1000; // In ms

        Sensor::Driver         mDriver;
        ::Driver::SwTimer      mTimer;
        float                  mPrevLevel;
        Interface::IBattLevel& mBattLevel;
        OS::Mutex              mDataMutex;
    }; /* class BatteryLevel */

} /* namespace Sensor */

#endif /* __SENSOR_BATTERY_LEVEL_HPP */
