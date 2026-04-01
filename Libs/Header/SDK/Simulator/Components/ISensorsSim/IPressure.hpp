/**
 ******************************************************************************
 * @file    IPressure.hpp
 * @date    11-march-2026
 * @author  Vlad Andriyash
 * @brief   IPressure interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_PRESSURE_INTERFACE_HPP
#define __I_PRESSURE_INTERFACE_HPP

namespace Interface
{
    class IPressure
    {
    public:
		/**
         * @brief   Destructor.
         */
		virtual ~IPressure() = default;
		
		/**
         * @brief   Set pressure simulation parameter.
         * @param   pressureValue: pressure value used for simulation in Pa.
         */
        virtual void setParam(float pressureValue) = 0;
		
		/**
         * @brief   Get simulated pressure value.
         * @retval  Pressure value in Pa.
         */
        virtual float getPressure() = 0;
    }; /* class IPressure */
} /* namespace Interface */

#endif /* __I_PRESSURE_INTERFACE_HPP */
