/**
 ******************************************************************************
 * @file    ISensor.hpp
 * @date    11-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   ISensor interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_SENSOR_INTERFACE_HPP
#define __I_SENSOR_INTERFACE_HPP

namespace Interface
{
    class ISensor
    {
    public:
		/**
         * @brief   Destructor.
         */
		virtual ~ISensor() = default;
	
        virtual void sensorRefresh() = 0;
    }; /* class ISensor */
} /* namespace Sensor */

#endif /* __I_SENSOR_INTERFACE_HPP */
