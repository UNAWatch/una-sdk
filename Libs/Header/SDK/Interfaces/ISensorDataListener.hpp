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
#include "SDK/Interfaces/ISensorData.hpp"

#include <vector>

namespace SDK::Interface
{
    
class ISensorDriver;    //// Forward declaration

class ISensorDataListener
{
public:
    virtual ~ISensorDataListener() = default;

    virtual void onSdlNewData(const SDK::Interface::ISensorDriver*             sensor,
                              const std::vector<SDK::Interface::ISensorData*>& data,
                              bool                                             first) = 0;
};

} // namespace SDK::Interface
