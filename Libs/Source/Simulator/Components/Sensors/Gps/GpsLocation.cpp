/**
 ******************************************************************************
 * @file    GpsLocation.cpp
 * @date    28-October-2025
 * @author Vlad
 * @brief   Sensor for the GPS Location
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Gps.Loc"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/Gps/GpsLocation.hpp"
#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

namespace Sensor
{

GpsLocation::GpsLocation()
    : mDriver(*this,
              SDK::Sensor::Type::GPS_LOCATION,
              SDK::SensorDataParser::GpsLocation::COUNT,
              *this)
    , mTimer()
    , mGps(ComponentSimulator::GetInstance().getGps())
    , mPoint0Inited(true)

{
}

Sensor::Driver& GpsLocation::getDriver()
{
    return mDriver;
}

float GpsLocation::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));
    mGps.enable();
    return period;
}

void GpsLocation::sdcStop(Sensor::Driver* driver)
{
    (void)driver;

    mTimer.stop();
    mGps.disable();
    LOG_INFO("stopped\n");
}

float GpsLocation::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float GpsLocation::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;

    return static_cast<float>(mMinPeriod);
}

const char* GpsLocation::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;

    return "Gps location";
}

void GpsLocation::sensorRefresh()
{
    if (!mTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    IGps::LocationInfo point = mGps.getLocation();

    if (point.valid) {
        if (!mPoint0Inited) {
            mPoint0Inited = true;
            mFilterLat.forceValue(point.lat),
            mFilterLon.forceValue(point.lon);
        }
        else {
            float distance = distanceFlatEarth(point,
                mFilterLat.execute(point.lat),
                mFilterLon.execute(point.lon));

            if (distance >= 2) {
                point.lat = mFilterLat.getValue();
                point.lon = mFilterLon.getValue();
            }
        }
    }
    
    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
    sample.f[SDK::SensorDataParser::GpsLocation::Field::PRECISION] = point.precision;
    sample.u[SDK::SensorDataParser::GpsLocation::Field::COORDS_VALID] = point.valid ? 1UL : 0UL;
    sample.f[SDK::SensorDataParser::GpsLocation::Field::LAT] = point.lat;
    sample.f[SDK::SensorDataParser::GpsLocation::Field::LON] = point.lon;
    sample.f[SDK::SensorDataParser::GpsLocation::Field::ALT] = point.alt;
    mDriver.pushDataSample();

    LOG_DEBUG("EXIT\n");
}

} /* namespace Sensor */
