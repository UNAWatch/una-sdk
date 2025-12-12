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

#pragma once

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/SensorData.hpp"

#include <vector>

namespace SDK::Interface
{
    
class ISensorDriver;    //// Forward declaration

class ISensorDataListener
{
public:
    virtual ~ISensorDataListener() = default;

    virtual void onSdlNewData(uint16_t                 handle,
                              const SDK::Sensor::Data* data,
                              uint16_t                 count,
                              uint16_t                 stride) = 0;
};

} // namespace SDK::Interface
