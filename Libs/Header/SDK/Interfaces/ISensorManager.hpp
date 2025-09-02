/**
 ******************************************************************************
 * @file    ISensorManager.hpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorManager interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_SENSOR_MANAGER_HPP
#define __I_SENSOR_MANAGER_HPP

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/ISensorDriver.hpp"

#include <vector>

namespace Interface
{
    class ISensorManager {
    public:

        /**
         * @brief Use this method to get the default sensor for a given type.
         * @note Returned sensor could be a composite sensor, and its data could
         *       be averaged or filtered. If you need to access the raw sensors
         *       use getSensorList.
         * @param type: Sensor requested type.
         * @return The default sensor matching the requested type if one exists,
         *         or null otherwise.
         */
        virtual Interface::ISensorDriver* getDefaultSensor(Sensor::Type type) = 0;

        /**
         * @brief Get the list of available sensors of a certain type.
         * @param type: Sensor requested type.
         * @return A list of sensors matching the asked type.
         */
        virtual std::vector<Interface::ISensorDriver*> getSensorList(Sensor::Type type) = 0;
    };

} /* namespace Sensor */

#endif /* __I_SENSOR_MANAGER_HPP */
