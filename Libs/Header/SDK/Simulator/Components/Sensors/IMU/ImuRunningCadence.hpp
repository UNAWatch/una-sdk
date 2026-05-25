/**
 ******************************************************************************
 * @file    ImuRunningCadence.hpp
 * @brief   Simulator driver for RUNNING_CADENCE sensor samples.
 ******************************************************************************
 */

#ifndef __IMU_RUNNING_CADENCE_HPP
#define __IMU_RUNNING_CADENCE_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>
#include <cstdint>

namespace Sensor
{

class ImuRunningCadence : public Interface::ISensor,
                          public Sensor::ISensorDriverCtrl
{
public:
    ImuRunningCadence();

    Sensor::Driver& getDriver();

    float       sdcStart(Sensor::Driver* driver, float period)        override;
    void        sdcStop(Sensor::Driver* driver)                       override;
    float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
    float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
    const char* sdcGetDescription(Sensor::Driver* driver)             override;

    void sensorRefresh() override;

private:
    static constexpr uint32_t mMinPeriodMs = 1000;
    static constexpr float    mSimCadenceSpm = 170.0f;

    void publishSample();

    Interface::IGps& mGps;
    Sensor::Driver   mDriver;
    ::Driver::SwTimer mTimer;
};

} // namespace Sensor

#endif /* __IMU_RUNNING_CADENCE_HPP */
