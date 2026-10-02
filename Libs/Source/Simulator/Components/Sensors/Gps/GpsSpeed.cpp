/**
 ******************************************************************************
 * @file    GpsSubSensorSpeed.cpp
 * @date    28-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SubSensor for the GPS Speed
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Gps.Speed"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/Gps/GpsSpeed.hpp"
#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsSpeed.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

namespace Sensor
{

GpsSpeed::GpsSpeed()
    : mDriver(*this,
              SDK::Sensor::Type::GPS_SPEED,
              SDK::SensorDataParser::GpsSpeed::COUNT,
              *this)
    , mTimer()
    , mGps(ComponentSimulator::GetInstance().getGps())
{
}

Sensor::Driver& GpsSpeed::getDriver()
{
    return mDriver;

}

float GpsSpeed::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));
    mGps.enable();
    return period;
}

void GpsSpeed::sdcStop(Sensor::Driver* driver)
{
    (void)driver;

    mTimer.stop();
    mGps.disable();
    LOG_INFO("stopped\n");
}

float GpsSpeed::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float GpsSpeed::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;

    return static_cast<float>(mMinPeriod);
}

const char* GpsSpeed::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;

    return "Gps speed";
}

void GpsSpeed::sensorRefresh()
{
    if (mTimer.check()) {
        LOG_DEBUG("ENTRY\n");

        auto& sample = mDriver.getDataSample();
        sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
        sample.f[SDK::SensorDataParser::GpsSpeed::Field::SPEED] = mGps.getSpeed();
        // The simulator's IGps has no fix-mode/dead-reckoning concept, so a fix
        // is always a reliable (non-DR) fix here.
        sample.u[SDK::SensorDataParser::GpsSpeed::Field::SPEED_VALID] =
            mGps.hasFix() ? 1u : 0u;
        sample.u[SDK::SensorDataParser::GpsSpeed::Field::DEAD_RECKONING] = 0u;
        mDriver.pushDataSample();

        LOG_DEBUG("EXIT\n");
    }
}

} /* namespace Sensor */
