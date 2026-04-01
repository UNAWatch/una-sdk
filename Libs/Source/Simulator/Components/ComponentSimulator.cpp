/**
 ******************************************************************************
 * @file    ComponentSimulator.cpp
 * @date    10-08-2025
 * @author  Vlad
 * @brief  
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "ComponentSimulator"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/ComponentSimulator.hpp"
#include "SDK/Simulator/Components/Simulator/GpsStepCounterSimulator.hpp"
#include "SDK/Simulator/Components/Simulator/BattLevelSimulator.hpp"
#include "SDK/Simulator/Components/Simulator/HeatRateSimulator.hpp"
#include "SDK/Simulator/Components/Simulator/PressureSimulator.hpp"

ComponentSimulator::ComponentSimulator()
{
    Simulator::GpsStepCounterSimulator* gpsSimStepCounter = new (std::nothrow) Simulator::GpsStepCounterSimulator();

    mpGps = gpsSimStepCounter;
    mpBattLevel  = new (std::nothrow) Simulator::BatteryLevelSimulator();
    mpHeartRate = new (std::nothrow) Simulator::HeartRateSimulator();
    mpPressure = new (std::nothrow) Simulator::PressureSimulator();
    mpStepCounter = gpsSimStepCounter;
}

void ComponentSimulator::setParamBatterySimulation(float startValue, float stepValue)
{
    mpBattLevel->setParam(startValue, stepValue);
}

void ComponentSimulator::setParamGpsSimulation(float speedMin, float speedMidle, float speedMax, uint32_t seachSatteliteMs)
{
    mpGps->setParamSimulation(speedMin, speedMidle, speedMax, seachSatteliteMs);
}

void ComponentSimulator::setParamHeartRateSimulation(uint8_t minHr, uint8_t maxHr, uint8_t typeTraining)
{
    mpHeartRate->setParam(minHr, maxHr, typeTraining);
}

void ComponentSimulator::setParamPressureSimulation(float pressureValue)
{
    mpPressure->setParam(pressureValue);
}

void ComponentSimulator::setParamStepCounterSimulation(float strideLength)
{
    mpStepCounter->setParamStepCounter(strideLength);
}

Interface::IGps& ComponentSimulator::getGps()
{
    return *mpGps;
}

Interface::IHeartRate& ComponentSimulator::getHeartRate()
{
    return *mpHeartRate;
}

Interface::IBattLevel& ComponentSimulator::getBatteryLevel()
{
    return *mpBattLevel;
}

Interface::IPressure& ComponentSimulator::getPressure()
{
    return *mpPressure;
}

Interface::IStepCounter& ComponentSimulator::getSteCounter()
{
    return *mpStepCounter;
}


