/**
 ******************************************************************************
 * @file    ImuWristMotion.hpp
 * @date    14-November-2025
 * @author  Vlad
 * @brief   Sensor for the accelerometer
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "IMU.WUP"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/IMU/ImuWristMotion.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>

namespace Sensor
{
ImuWristMotion::ImuWristMotion()
    : mDriver(*this,
              SDK::Sensor::Type::WRIST_MOTION,
              SDK::SensorDataParser::WristMotion::getFieldsNumber(),
              *this)
    , mTimer()
    , mQueue()
{
    mQueue.init(LOG_MODULE_PRX);
}

Sensor::Driver& ImuWristMotion::getDriver()
{
    return mDriver;
}

float ImuWristMotion::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

void ImuWristMotion::sdcStop(Sensor::Driver* driver)
{
    (void)driver;

    mTimer.stop();

    LOG_INFO("stopped\n");
}

float ImuWristMotion::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    mTimer.start(static_cast<uint32_t>(period));

    return period;
}

float ImuWristMotion::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;

    return static_cast<float>(mMinPeriod);
}

const char* ImuWristMotion::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;

    return "Wrist motion";
}

void ImuWristMotion::sensorRefresh()
{
    if (!mTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    if (mQueue.empty()) {
        LOG_DEBUG("EXIT\n");
        return;
    }

    uint64_t ts;
    mQueue.pop(ts);

    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(static_cast<uint32_t>(ts));
    sample.u[SDK::SensorDataParser::WristMotion::Field::WRIST_MOTION] = 1;
    mDriver.pushDataSample();

    LOG_DEBUG("EXIT\n");;
}

void ImuWristMotion::handleWristMotion(uint64_t timestamp)
{
    if (mTimer.isActive()) {
        LOG_INFO("detected\n");
        mQueue.push(timestamp);
    }
}

} /* namespace Sensor */
