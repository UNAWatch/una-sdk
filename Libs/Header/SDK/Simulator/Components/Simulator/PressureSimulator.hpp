/**
 ******************************************************************************
 * @file    PressureSimulator.hpp
 * @date    20-March-2026
 * @author  Vlad Andriyash
 * @brief   Simulator for the Battery Level
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __PRESSURE_SIMULATOR_HPP
#define __PRESSURE_SIMULATOR_HPP

#include <iostream>
#include <random>
#include <chrono>
#include "SDK/Simulator/Components/ISensorsSim/IPressure.hpp"


namespace Simulator
{
    class PressureSimulator : public Interface::IPressure
    {
    public:
        PressureSimulator()
            : mNoise(0.0, mNoiseLevel)
        { 
            mGenerator.seed(std::random_device{}());
        }

        ~PressureSimulator() = default;

        //// IPressure
        virtual void setParam(float pressureValue) override
        {
            mP0 = pressureValue;
        }

        virtual float getPressure() override
        {
            return genetatePressure();
        }

    private:

		/**
		 * @brief   Generate next value Pressure.
		 * @retval  pressure value in Pa.
		 */
        float genetatePressure()
        {
            return static_cast<float>(mP0 + mNoise(mGenerator));
        }
      
        /* 1 sigma, hPa. Altitude comes from the pressure ratio, so the old 0.3
           was ~2.5 m of noise -- past the apps' 2 m ascent threshold, which made
           flat rides accumulate gain. ~3 Pa matches a real MS5837. */
        float mNoiseLevel = 0.03f;
        float mP0 = 1013.25f;
        std::default_random_engine mGenerator;
        std::normal_distribution<double> mNoise;
    };
}

#endif /* __PRESSURE_SIMULATOR_HPP */