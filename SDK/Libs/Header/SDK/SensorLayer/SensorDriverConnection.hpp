/**
 ******************************************************************************
 * @file    SensorDriverConnection.cpp
 * @date    17-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Connection to the SensorDriver
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDriver.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"

namespace SDK::Sensor {

    class DriverConnection {
    public:
        DriverConnection();

        DriverConnection(SDK::Sensor::Type                    id,
                         SDK::Interface::ISensorDataListener* listener,
                         float                                period  = 0,
                         uint32_t                             latency = 0);

        DriverConnection(SDK::Interface::ISensorDriver*       driver,
                         SDK::Interface::ISensorDataListener* listener,
                         float                                period  = 0,
                         uint32_t                             latency = 0);

        DriverConnection(SDK::Interface::ISensorDriver*       driver,
                         SDK::Interface::ISensorDataListener* listener,
                         SDK::Interface::IApp*                app,
                         float                                period  = 0,
                         uint32_t                             latency = 0);

        virtual ~DriverConnection() = default;

        bool isValid();

        bool connect();
        bool connect(float period, uint32_t latency);
        bool connect(SDK::Interface::ISensorDriver*       driver,
                     SDK::Interface::ISensorDataListener* listener,
                     float                                period = 0,
                     uint32_t                             latency = 0);

        bool isConnected();

        void disconnect();
        
        bool matchesDriver(const SDK::Interface::ISensorDriver* driver);

    protected:
        SDK::Interface::ISensorDriver*       mDriver;
        SDK::Interface::ISensorDataListener* mListener;
        float                                mPeriod;
        uint32_t                             mLatency;
        SDK::Interface::IApp*                mUserApp;
        bool                                 mIsConnected;
    };

} // namespace SDK::Sensor
