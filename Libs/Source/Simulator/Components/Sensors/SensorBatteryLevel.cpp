/**
 ******************************************************************************
 * @file    SensorBatteryLevel.cpp
 * @date    23-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor Battery Level
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Sensor.BattLevel"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/SensorBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include <SDK/Simulator/Kernel/Mock/System.hpp>
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

#include <inttypes.h>

namespace Sensor
{

    BatteryLevel::BatteryLevel()
        : mDriver(*this,
            SDK::Sensor::Type::BATTERY_LEVEL,
            SDK::SensorDataParser::BatteryLevel::getFieldsNumber(),
            *this,
            Sensor::Driver::Mode::EVENT_BASED)
        , mTimer(1000)
        , mDataMutex()
        , mPrevLevel(0)
        , mBattLevel(ComponentSimulator::GetInstance().getBatteryLevel())
{
}

Sensor::Driver& BatteryLevel::getDriver()
{
    return mDriver;
}

float BatteryLevel::sdcStart(Sensor::Driver* driver, float period)
{
    (void) driver;
    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

void BatteryLevel::sdcStop(Sensor::Driver* driver)
{
    (void) driver;

    mTimer.stop();

    LOG_INFO("stopped\n");
}

float BatteryLevel::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void) driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float BatteryLevel::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void) driver;

    return static_cast<float>(mMinPeriod);
}

const char* BatteryLevel::sdcGetDescription(Sensor::Driver* driver)
{
    (void) driver;

    return "Battery level";
}

void BatteryLevel::sensorRefresh()
{
    static bool first = false;
    // Send data one time after connect
    if (!first) {
        first = true;
        publishData(mBattLevel.getBattLevel());
        return;
    }

    if (!mTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    float level = mBattLevel.getBattLevel();

    OS::MutexCS cs(mDataMutex);
    if (mPrevLevel > 0 && abs(mPrevLevel - level) < 0.1) {
        LOG_DEBUG("EXIT\n");
        return;
    }

    mPrevLevel = level;

    publishData(level);

    LOG_DEBUG("EXIT\n");
}

void BatteryLevel::publishData(float level)
{
    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
    sample.f[SDK::SensorDataParser::BatteryLevel::LEVEL] = level;
    mDriver.pushDataSample();
}

} /* namespace Sensor */