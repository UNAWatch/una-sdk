/**
 ******************************************************************************
 * @file    SensorHeartRateMetrics.cpp
 * @date    15-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Additional metrics from the heart rate
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "SensorHRMetrics"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/HeartRate/SensorHeartRateMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateMetrics.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

namespace Sensor
{

HeartRateMetrics::HeartRateMetrics()
    : mDriver(*this,
              SDK::Sensor::Type::HEART_RATE_METRICS,
              SDK::SensorDataParser::HeartRateMetrics::getFieldsNumber(),
              *this)
    , mpHeatRateSim(ComponentSimulator::GetInstance().getHeartRate())
    , mUpdateTimer(0)
    , mData()
{
}

Sensor::Driver& HeartRateMetrics::getDriver()
{
    return mDriver;
}

float HeartRateMetrics::sdcStart(Sensor::Driver* driver, float period)
{
    LOG_INFO("start\n");

    mUpdateTimer.start(static_cast<uint32_t>(period));

    return period;
}

void HeartRateMetrics::sdcStop(Sensor::Driver* driver)
{
    LOG_INFO("stop\n");

    mUpdateTimer.stop();
}

float HeartRateMetrics::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    mUpdateTimer.start(static_cast<uint32_t>(period));
    return period;
}

float HeartRateMetrics::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void) driver;

    return mMinPeriod;
}

const char* HeartRateMetrics::sdcGetDescription(Sensor::Driver* driver)
{
    (void) driver;

    return "Heart rate metrics";
}

void HeartRateMetrics::sensorRefresh()
{
    if (!mUpdateTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    mpHeatRateSim.nextHR();

    mData.ahr = mpHeatRateSim.getAHR();
    mData.rhr = mpHeatRateSim.getRHR();

    publishData();

    LOG_DEBUG("EXIT\n");
}

void HeartRateMetrics::publishData()
{
    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
    sample.f[SDK::SensorDataParser::HeartRateMetrics::Field::AHR] = mData.ahr;
    sample.f[SDK::SensorDataParser::HeartRateMetrics::Field::RHR] = mData.rhr;
    mDriver.pushDataSample();
}



} /* namespace Sensor */
