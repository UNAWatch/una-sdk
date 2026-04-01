/**
 ******************************************************************************
 * @file    BodySubSensorHeartRate.cpp
 * @date    05-January-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SubSensor for the Heart Rate
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "HeartRate"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/HeartRate/SensorHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometerRaw.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

using namespace Interface;

namespace Sensor
{

HeartRate::HeartRate()
    : mDriver(*this,
              SDK::Sensor::Type::HEART_RATE,
              SDK::SensorDataParser::HeartRate::getFieldsNumber(),
              *this)
    , mpHeatRateSim(ComponentSimulator::GetInstance().getHeartRate())
    , mTimer()
    , mHr(0)
    , mTrustLevel(0)
{
}

Sensor::Driver& HeartRate::getDriver()
{
    return mDriver;
}

float HeartRate::sdcStart(Sensor::Driver* driver, float period)
{
    LOG_INFO("start\n");
    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

void HeartRate::sdcStop(Sensor::Driver* driver)
{
    LOG_INFO("stop\n");
    mTimer.stop();
}

float HeartRate::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    mTimer.start(static_cast<uint32_t>(period));
    return period;
}

float HeartRate::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;
    return mMinPeriod;
}

const char* HeartRate::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;
    return "Heart Rate sensor";
}

void HeartRate::sensorRefresh()
{
    if (!mTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    float hr;
    float trustLevel;

    hr = static_cast<float>(mpHeatRateSim.nextHR());
    trustLevel = static_cast<float>(mpHeatRateSim.getTrustLevel());

    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
    sample.f[SDK::SensorDataParser::HeartRate::Field::BPM]         = hr;
    sample.f[SDK::SensorDataParser::HeartRate::Field::TRUST_LEVEL] = trustLevel;
    mDriver.pushDataSample();

    LOG_DEBUG("EXIT\n");
}

} /* namespace Sensor */
