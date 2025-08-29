/**
 ******************************************************************************
 * @file    ISensorDataListener.hpp
 * @date    28-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Interface for receiving notifications when new sensor data
 *          is available.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */


#ifndef __I_SENSOR_DATA_LISTENER_HPP
#define __I_SENSOR_DATA_LISTENER_HPP

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/ISensorData.hpp"

#include <vector>

namespace Interface
{
    class ISensorDriver;    //// Forward declaration

    class ISensorDataListener
    {
    public:
        virtual ~ISensorDataListener() = default;

        virtual void onNewSensorData(const Interface::ISensorDriver*             sensor,
                                     const std::vector<Interface::ISensorData*>& data,
                                     bool                                        first) = 0;
    }; /* class ISensorDataListener */

} /* namespace Interface */

#endif /* __I_SENSOR_DATA_LISTENER_HPP */
