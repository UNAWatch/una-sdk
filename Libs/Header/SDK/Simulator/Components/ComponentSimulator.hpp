/**
 ******************************************************************************
 * @file    ComponentSimulator.hpp
 * @date    10-08-2025
 * @author  Vlad
 * @brief   
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __COMPONENT_SIMULATOR_HPP
#define __COMPONENT_SIMULATOR_HPP

#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IBatteryLevel.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IHeatRate.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IPressure.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IStepCounter.hpp"

/**
 * @brief   To create components that present in the system.
 */
class ComponentSimulator
{

public:

    /**
     * @brief   Get ComponentSimulator instance.
     * @retval  ComponentSimulator instance.
     */
    static ComponentSimulator& GetInstance()
    {
        static ComponentSimulator sInstance;
        return sInstance;
    }

    void setParamBatterySimulation(float startValue, float stepValue);
    void setParamGpsSimulation(float speedMin, float speedMidle, float speedMax, uint32_t seachSatteliteMs);
    void setParamHeartRateSimulation(uint8_t minHr, uint8_t maxHr, uint8_t typeTraining);
    void setParamPressureSimulation(float pressureValue);
    void setParamStepCounterSimulation(float strideLength);

    Interface::IGps& getGps();
    Interface::IHeartRate& getHeartRate();
    Interface::IBattLevel& getBatteryLevel();
    Interface::IPressure& getPressure();
    Interface::IStepCounter& getSteCounter();

protected:

    /**
     * @brief   Constructor.
     */
    ComponentSimulator();

    /**
     * @brief   Destructor.
     */
    virtual ~ComponentSimulator() = default;

private:

    Interface::IGps*         mpGps = nullptr;
    Interface::IBattLevel*   mpBattLevel = nullptr;
    Interface::IHeartRate*   mpHeartRate = nullptr;
    Interface::IPressure*    mpPressure = nullptr;
    Interface::IStepCounter* mpStepCounter = nullptr;

};

#endif /* __COMPONENT_SIMULATOR_HPP */
