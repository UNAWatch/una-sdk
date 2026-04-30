/**
 ******************************************************************************
 * @file    IStepCounter.hpp
 * @date    11-March-2026
 * @author  Vlad Andriyash
 * @brief   IStepCounter interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_STEP_COUNTER_INTERFACE_HPP
#define __I_STEP_COUNTER_INTERFACE_HPP

namespace Interface
{
    class IStepCounter
    {
    public:
		/**
         * @brief   Destructor.
         */
		virtual ~IStepCounter() = default;
	
		/**
         * @brief   Start step counter simulation.
         * @details Enables generation of simulated step data.
         */
        virtual void startStepCounter() = 0;

        /**
         * @brief   Stop step counter simulation.
         * @details Stops generation of simulated step data.
         */
        virtual void stopStepCounter() = 0;

        /**
         * @brief   Set parameters for step counter simulation.
         * @param   strideLength: simulated stride length in meters.
         */
        virtual void setParamStepCounter(float strideLength) = 0;

        /**
         * @brief   Get current simulated step counter value.
         * @retval  Total number of simulated steps.
         */
        virtual uint32_t getStepCounter() = 0;
    }; /* class IStepCouner */
} /* namespace Interface */

#endif /* __I_STEP_COUNTER_INTERFACE_HPP */
