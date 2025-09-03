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
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Interfaces/IUserApp.hpp"

namespace SDK::Interface
{
    class ISensorDriver {
    public:
        ISensorDriver() : mPeriod(1000) {}
        virtual ~ISensorDriver() = default;

        virtual bool connect(SDK::Interface::ISensorDataListener* listener,
                             SDK::Interface::IUserApp*            userapp = nullptr,
                             float                                period  = 0,
                             uint32_t                             latency = 0) = 0;

        virtual void disconnect(SDK::Interface::ISensorDataListener* listener) = 0;
        
        virtual SDK::Sensor::Type getType() = 0;

        virtual float getPeriod() { return mPeriod; }

    protected:
        float mPeriod;
    };

} /* namespace Sensor */

#endif /* __I_SENSOR_DRIVER_HPP */
