/**
 ******************************************************************************
 * @file    SensorConnection.hpp
 * @date    11-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Connection to the SensorDriver using messages
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"

namespace SDK::Sensor {

    class Connection {
    public:
        Connection(SDK::Sensor::Type id,
                   float             period  = 0,
                   uint32_t          latency = 0);

        Connection(uint8_t  handle,
                   float    period  = 0,
                   uint32_t latency = 0);

        ~Connection();

        bool isValid();

        bool connect();
        bool connect(float period, uint32_t latency = 0);

        bool isConnected();

        void disconnect();
        
        bool matchesDriver(uint16_t handle);

    protected:
        bool subscribe();

        SDK::Kernel&      mKernel;
        SDK::Sensor::Type mID;
        uint8_t           mHandle;
        float             mPeriod;
        uint32_t          mLatency;
        bool              mIsConnected;
    };

} // namespace SDK::Sensor
