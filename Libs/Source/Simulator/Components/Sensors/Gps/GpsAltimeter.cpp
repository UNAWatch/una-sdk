/**
 ******************************************************************************
 * @file    GpsAltimeter.cpp
 * @date    10-November-2025
 * @author  Vlad
 * @brief   Sensor for the GPS Altimeter
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Gps.Alt"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/Gps/GpsAltimeter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAltimeter.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include <SDK/Simulator/Components/ComponentSimulator.hpp>

namespace Sensor
{

GpsAltimeter::GpsAltimeter()
    : mDriver(*this,
              SDK::Sensor::Type::ALTIMETER,
              SDK::SensorDataParser::Altimeter::getFieldsNumber(),
              *this)
    , mTimer()
    , mData()
    , mDataMutex()
    , mGps(ComponentSimulator::GetInstance().getGps())
{
}

Sensor::Driver& GpsAltimeter::getDriver()
{
    return mDriver;
}

float GpsAltimeter::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));
    mGps.enable();
    return period;
}

void GpsAltimeter::sdcStop(Sensor::Driver* driver)
{
    (void)driver;

    mTimer.stop();
    mGps.disable();
    LOG_INFO("stopped\n");
}

float GpsAltimeter::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float GpsAltimeter::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;

    return static_cast<float>(mMinPeriod);
}

const char* GpsAltimeter::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;

    return "Gps altimeter";
}

void GpsAltimeter::sensorRefresh()
{
    if (mTimer.check()) {
        LOG_DEBUG("ENTRY\n");

        OS::MutexCS cs(mDataMutex);

        auto& sample = mDriver.getDataSample();
        sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
        sample.f[SDK::SensorDataParser::Altimeter::Field::ALTITUDE] = mGps.getAltitude();
        mDriver.pushDataSample();

        LOG_DEBUG("EXIT\n");
    }
}

} /* namespace Sensor */
