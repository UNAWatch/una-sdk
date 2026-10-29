/**
 ******************************************************************************
 * @file    ImuStepCounter.cpp
 * @date    14-March-2026
 * @author  Vlad
 * @brief   Sensor for the step counter
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "IMU.StepCnt"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/IMU/ImuStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include <SDK/Simulator/Kernel/Mock/System.hpp>
#include "SDK/Simulator/Components/ComponentSimulator.hpp"

namespace Sensor
{
    ImuStepCounter::ImuStepCounter()
    : mStepCounterSim(ComponentSimulator::GetInstance().getSteCounter())
    , mDriver(*this,
              SDK::Sensor::Type::STEP_COUNTER,
              SDK::SensorDataParser::StepCounter::getFieldsNumber(),
              *this,
              Sensor::Driver::Mode::EVENT_BASED)
    , mDailySensor(*this)
    , mDailyDriver(mDailySensor,
              SDK::Sensor::Type::STEP_COUNTER_DAILY,
              SDK::SensorDataParser::StepCounter::getFieldsNumber(),
              *this,
              Sensor::Driver::Mode::EVENT_BASED)
    , mTimer()
    , mDataMutex()
{
}

Sensor::Driver& ImuStepCounter::getDriver()
{
    return mDriver;
}

Sensor::Driver& ImuStepCounter::getDailyDriver()
{
    return mDailyDriver;
}

float ImuStepCounter::sdcStart(Sensor::Driver* driver, float period)
{
    LOG_INFO("start\n");

    if (mActiveDrivers++ == 0) {
        mStepCounterSim.startStepCounter();
        mFirstSample = true;
        mTimer.start(mMinPeriod);
    }

    return period;
}

void ImuStepCounter::sdcStop(Sensor::Driver* driver)
{
    LOG_INFO("stop\n");

    if (mActiveDrivers && --mActiveDrivers == 0) {
        mTimer.stop();
        mStepCounterSim.stopStepCounter();
    }
}

float ImuStepCounter::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    return period;
}

float ImuStepCounter::sdcGetMinPeriod(Sensor::Driver* driver)
{
    return static_cast<float>(mMinPeriod);
}

const char* ImuStepCounter::sdcGetDescription(Sensor::Driver* driver)
{
    return "Step counter";
}

void ImuStepCounter::sensorRefresh()
{
    // Send data one time after connect
    if (mFirstSample) {
        mFirstSample = false;
        publishData(mStepCounterSim.getStepCounter());
        return;
    }

    if (!mTimer.check()) {
        return;
    }

    LOG_DEBUG("ENTRY\n");

    publishData(mStepCounterSim.getStepCounter());

    LOG_DEBUG("EXIT\n");
}

void ImuStepCounter::publishData(uint32_t value)
{
    for (auto* driver : {&mDriver, &mDailyDriver}) {
        auto& sample = driver->getDataSample();
        sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
        sample.u[SDK::SensorDataParser::StepCounter::Field::STEP_COUNT] = value;
        driver->pushDataSample();
    }
}

} /* namespace Sensor */
