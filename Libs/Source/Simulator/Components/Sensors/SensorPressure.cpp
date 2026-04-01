/**
 ******************************************************************************
 * @file    MS5837SubSensorP.cpp
 * @date    14-November-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SubSensor for the accelerometer
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Pressure"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/SensorPressure.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserPressure.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

namespace Sensor
{
    Pressure::Pressure()
    : mDriver(*this,
              SDK::Sensor::Type::PRESSURE,
              SDK::SensorDataParser::Pressure::getFieldsNumber(),
              *this)
    , mGps(ComponentSimulator::GetInstance().getGps())
    , mPressure(ComponentSimulator::GetInstance().getPressure())
    , mTimer()
{
}

Sensor::Driver& Pressure::getDriver()
{
    return mDriver;
}

float Pressure::sdcStart(Sensor::Driver* driver, float period)
{
    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

void Pressure::sdcStop(Sensor::Driver* driver)
{
    LOG_INFO("stop\n");

    mTimer.stop();
}

float Pressure::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float Pressure::sdcGetMinPeriod(Sensor::Driver* driver)
{
    return static_cast<float>(mMinPeriod);
}

const char* Pressure::sdcGetDescription(Sensor::Driver* driver)
{
    return "MS5837 pressure";
}

void Pressure::sensorRefresh()
{
    if (mTimer.check()) {
        LOG_DEBUG("ENTRY\n");

        float pressure = mPressure.getPressure();
        float altitude = mGps.getAltitude();

        pressure *= 100; //convert to Pascals
        float value = computeP0FromAltitude(pressure, altitude);
        
        //if (mProvider.getPressure(pressure)) {
            auto& sample = mDriver.getDataSample();
            sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
            sample.f[SDK::SensorDataParser::Pressure::PRESS]           = pressure;
            sample.f[SDK::SensorDataParser::Pressure::PRESS_SEA_LEVEL] = value;
            mDriver.pushDataSample();
       // }

        LOG_DEBUG("EXIT\n");
    }
}



float Pressure::computeP0FromAltitude(float p, float altitude)
{
    // Validity checks
    if (!(p > 0.0f)) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Model domain: h must be < 44330 m for (1 - h/44330) > 0
    // (That’s well above any hiking use-case anyway)
    const float x = 1.0f - (altitude / 44330.0f);
    if (!(x > 0.0f)) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    static constexpr float n = 5.255877f; // = 1 / inv_n

    return p / std::pow(x, n);
}

} /* namespace Sensor */
