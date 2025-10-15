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

#pragma once

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDriver.hpp"

#include <vector>

namespace SDK::Interface
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
    virtual SDK::Interface::ISensorDriver* getDefaultSensor(SDK::Sensor::Type type) = 0;

    /**
     * @brief Get the list of available sensors of a certain type.
     * @param type: Sensor requested type.
     * @return A list of sensors matching the asked type.
     */
    virtual std::vector<SDK::Interface::ISensorDriver*> getSensorList(SDK::Sensor::Type type) = 0;
};

} // namespace SDK::Interface
