/**
 ******************************************************************************
 * @file    IHeartRate.hpp
 * @date    11-March-2026
 * @author  Vlad Andriyash
 * @brief   IHeartRate interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_HEART_RATE_INTERFACE_HPP
#define __I_HEART_RATE_INTERFACE_HPP

namespace Interface
{
    class IHeartRate
    {
    public:
		/**
         * @brief   Destructor.
         */
		virtual ~IHeartRate() = default;
		
        /**
         * @brief   Configure heart rate simulation parameters.
         * @param   minHr: minimum heart rate value for the simulation (BPM).
         * @param   maxHr: maximum heart rate value for the simulation (BPM).
         * @param   typeTraining: type of training scenario 
         *                        (e.g. 0 - cycling, 1 - hiking, 2- running).
         */
        virtual void setParam(uint8_t minHr, uint8_t maxHr, uint8_t typeTraining) = 0;

        /**
         * @brief   Generate next heart rate value.
         * @retval  Simulated heart rate value in BPM.
         */
        virtual int nextHR() = 0;

        /**
         * @brief   Get signal trust level of the last heart rate measurement.
         * @retval  Trust level (0 - low quality signal, 3 - high quality signal).
         */
        virtual int getTrustLevel() = 0;

        /**
         * @brief   Get simulated resting heart rate (RHR).
         * @retval  Resting heart rate value in BPM.
         */
        virtual float getRHR() = 0;

        /**
         * @brief   Get average heart rate (AHR) calculated during simulation.
         * @retval  Average heart rate value in BPM.
         */
        virtual float getAHR() = 0;
		
    }; /* class IHeartRate */
} /* namespace Interface */

#endif /* __I_HEART_RATE_INTERFACE_HPP */
