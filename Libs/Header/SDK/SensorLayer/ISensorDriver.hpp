/**
 ******************************************************************************
 * @file    ISensorDriver.cpp
 * @date    01-Audust-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   ISensorDriver interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_SENSOR_DRIVER_HPP
#define __I_SENSOR_DRIVER_HPP

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/ISensorDataListener.hpp"
#include "SDK/Interfaces/IUserApp.hpp"

namespace Interface
{
    class ISensorDriver {
    public:
        virtual bool connect(Interface::ISensorDataListener* listener,
                             Interface::IUserApp*            userapp = nullptr,
                             float                           period  = 0,
                             uint32_t                        latency = 0) = 0;

        virtual void disconnect(Interface::ISensorDataListener* listener) = 0;
        
        virtual Sensor::Type getType() = 0;
    };

} /* namespace Sensor */

#endif /* __I_SENSOR_DRIVER_HPP */
