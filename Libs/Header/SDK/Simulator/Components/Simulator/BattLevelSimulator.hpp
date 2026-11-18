/**
 ******************************************************************************
 * @file    BattLevelSimulator.hpp
 * @date    20-March-2026
 * @author  Vlad Andriyash
 * @brief   Simulator for the Battery Level
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __BATT_LEVEL_SIMULATOR_HPP
#define __BATT_LEVEL_SIMULATOR_HPP

#include <iostream>
#include <random>
#include <chrono>
#include "SDK/Simulator/Components/ISensorsSim/IBatteryLevel.hpp"
#include "SDK/Simulator/Automation/Overrides.hpp"


namespace Simulator
{
    class BatteryLevelSimulator : public Interface::IBattLevel
    {
    public:
        BatteryLevelSimulator() = default;
        ~BatteryLevelSimulator() = default;

        //// IBattLevel
        virtual void setParam(float startValue, float stepValue) override
        {
            mStepValue = stepValue;
            mBattValue = startValue;
        }

        virtual float getBattLevel() override
        {
            float percent = 0.0f;
            if (SDK::Simulator::Automation::Overrides::get().battery(percent)) {
                return percent;   // set by a simulator script
            }
            return generateBatteryLevel();
        }

    private:

		/**
		 * @brief   Generate next value battery level.
		 * @retval  battery level in percent.
		 */
        float generateBatteryLevel()
        {
            mBattValue -= mStepValue;

            if (mBattValue < 0.5 || mBattValue > 100.0) {
                mBattValue = 100.0;
            }

            return mBattValue;
        }
      
        float mStepValue = 0.1f;
        float mBattValue = 100.0f;
    };
}

#endif /* __BATT_LEVEL_SIMULATOR_HPP */