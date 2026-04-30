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
    : mDriver(*this,
              SDK::Sensor::Type::STEP_COUNTER,
              SDK::SensorDataParser::StepCounter::getFieldsNumber(),
              *this,
              Sensor::Driver::Mode::EVENT_BASED)
    , mTimer()
    , mDataMutex()
    , mStepCounterSim(ComponentSimulator::GetInstance().getSteCounter())
{
}

Sensor::Driver& ImuStepCounter::getDriver()
{
    return mDriver;
}

float ImuStepCounter::sdcStart(Sensor::Driver* driver, float period)
{
    LOG_INFO("start\n");

    mStepCounterSim.startStepCounter();

    mTimer.start(mMinPeriod);

    return period;
}

void ImuStepCounter::sdcStop(Sensor::Driver* driver)
{
    LOG_INFO("stop\n");

    mTimer.stop();

    mStepCounterSim.stopStepCounter();
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
    static bool first = false;
    // Send data one time after connect
    if (!first) {
        first = true;
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
    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());
    sample.u[SDK::SensorDataParser::StepCounter::Field::STEP_COUNT] = value;
    mDriver.pushDataSample();
}

} /* namespace Sensor */
