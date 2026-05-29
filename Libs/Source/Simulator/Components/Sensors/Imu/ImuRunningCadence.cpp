/**
 ******************************************************************************
 * @file    ImuRunningCadence.cpp
 * @brief   Simulator driver for RUNNING_CADENCE sensor samples.
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "IMU.RunCad"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/Sensors/IMU/ImuRunningCadence.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserRunningCadence.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"

namespace Sensor
{

ImuRunningCadence::ImuRunningCadence()
    : mDriver(*this,
              SDK::Sensor::Type::RUNNING_CADENCE,
              SDK::SensorDataParser::RunningCadence::getFieldsNumber(),
              *this)
    , mTimer()
{
}

Sensor::Driver& ImuRunningCadence::getDriver()
{
    return mDriver;
}

float ImuRunningCadence::sdcStart(Sensor::Driver* driver, float period)
{
    (void)driver;

    LOG_INFO("start\n");
    mTimer.start(mMinPeriodMs);
    return static_cast<float>(mMinPeriodMs);
}

void ImuRunningCadence::sdcStop(Sensor::Driver* driver)
{
    (void)driver;

    LOG_INFO("stop\n");
    mTimer.stop();
}

float ImuRunningCadence::sdcUpdatePeriod(Sensor::Driver* driver, float period)
{
    (void)driver;

    const float effectivePeriod =
        (period < static_cast<float>(mMinPeriodMs)) ? static_cast<float>(mMinPeriodMs) : period;
    mTimer.start(static_cast<uint32_t>(effectivePeriod));
    return effectivePeriod;
}

float ImuRunningCadence::sdcGetMinPeriod(Sensor::Driver* driver)
{
    (void)driver;
    return static_cast<float>(mMinPeriodMs);
}

const char* ImuRunningCadence::sdcGetDescription(Sensor::Driver* driver)
{
    (void)driver;
    return "Running cadence";
}

void ImuRunningCadence::sensorRefresh()
{
    if (!mTimer.check()) {
        return;
    }

    publishSample();
}

void ImuRunningCadence::publishSample()
{
    using Field = SDK::SensorDataParser::RunningCadence::Field;

    auto& sample = mDriver.getDataSample();
    sample.setTimestamp(SDK::Simulator::Mock::System::GetTimeMs());

    const bool cadenceValid = (mSimCadenceSpm > 0.0f);
    sample.f[Field::CADENCE_SPM] = mSimCadenceSpm;
    sample.u[Field::CADENCE_VALID] = cadenceValid ? 1u : 0u;

    // Step length is derived SDK-side from GPS speed + cadence at record-write
    // time (Outdoor-Data-Collection.md §3.6); the cadence sensor emits cadence
    // only, so it is no longer published here.

    mDriver.pushDataSample();
}

} // namespace Sensor
