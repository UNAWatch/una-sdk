/**
 ******************************************************************************
 * @file    GpsDistance.cpp
 * @date    28-October-2025
 * @author  Vlad
 * @brief   Sensor for the GPS Distance
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Gps.Distance"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/Gps/GpsDistance.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsDistance.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

using namespace Interface;

namespace Sensor
{

GpsDistance::GpsDistance()
    : mDriver(*this,
              SDK::Sensor::Type::GPS_DISTANCE,
              SDK::SensorDataParser::GpsDistance::getFieldsNumber(),
              *this)
    , mTimer()
    , mDistance(0.0f)
    , mGpsPoint0()
    , mGpsPoint0Valid(false)
    , mPoints()
    , mGps(ComponentSimulator::GetInstance().getGps())

{
    mPoints.init(LOG_MODULE_PRX);
}

Sensor::Driver& GpsDistance::getDriver()
{
    return mDriver;
}

float GpsDistance::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));
    mGps.enable();
    return period;
}

void GpsDistance::sdcStop(Sensor::Driver* driver)
{
    (void)driver;
    mGps.disable();
    mTimer.stop();

    LOG_INFO("stopped\n");
}

float GpsDistance::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float GpsDistance::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;

    return static_cast<float>(mMinPeriod);
}

const char* GpsDistance::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;

    return "Gps distance";
} 

void GpsDistance::sensorRefresh()
{
    static float distance = 0.0;
    if (mTimer.check()) {
        LOG_DEBUG("ENTRY\n");

        IGps::LocationInfo p = mGps.getLocation();
        if (p.valid) {
            GpsDistance::GpsPoint point(p.lat, p.lon);
            mPoints.push(point);
        };

        GpsDistance::GpsPoint point;
        if (mPoints.pop(point, 0)) {
            if (!mGpsPoint0Valid) {
                mGpsPoint0Valid = true;
                mGpsPoint0.latitude = point.latitude;
                mGpsPoint0.longitude = point.longitude;
            }
            else {
                mDistance += distanceHaversine(mGpsPoint0, point);
                mGpsPoint0 = point;
            }
        }

        auto& sample = mDriver.getDataSample();
        sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
        sample.f[SDK::SensorDataParser::GpsDistance::Field::DISTANCE] = mDistance;
        mDriver.pushDataSample();

        LOG_DEBUG("EXIT\n");
    }
}

} /* namespace Sensor */
