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

#pragma once

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Interfaces/IApp.hpp"

namespace SDK::Interface
{
    
class ISensorDriver {
public:
    ISensorDriver() : mRefreshPeriod(1000) {}
    virtual ~ISensorDriver() = default;

    virtual bool connect(SDK::Interface::ISensorDataListener* listener,
                            SDK::Interface::IApp*                app = nullptr,
                            float                                period  = 0,
                            uint32_t                             latency = 0) = 0;

    virtual void disconnect(SDK::Interface::ISensorDataListener* listener) = 0;
    
    virtual SDK::Sensor::Type getType() const = 0;

    virtual float getRefreshPeriod() const { return mRefreshPeriod; }

protected:
    float mRefreshPeriod;
};

} // namespace SDK::Interface
