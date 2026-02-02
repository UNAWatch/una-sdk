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

    /**
     * @brief High-level connection wrapper for sensor drivers.
     *
     * @details
     * The Connection class encapsulates IPC-based interaction with the SensorLayer.
     * It is responsible for:
     *  - Resolving a sensor driver handle via RequestDefault
     *  - Establishing a connection via RequestConnect
     *  - Releasing the connection via RequestDisconnect
     *
     * Internally, all communication is performed through Kernel IPC messages.
     *
     */
    class Connection {
    public:
        /**
         * @brief Construct a connection wrapper using a sensor type identifier.
         *
         * @details
         * This constructor stores the sensor type and desired connection parameters.
         * The actual sensor driver handle is resolved lazily during the first
         * call to @ref connect() using an IPC RequestDefault message.
         *
         * @param id       Sensor type used to resolve the default driver.
         * @param period   Desired sampling/update period (units defined by the driver).
         * @param latency  Maximum allowed reporting latency (units defined by the driver).
         *
         * @note Use @ref isValid() to check whether a driver handle has been resolved.
         */
        Connection(SDK::Sensor::Type id,
                   float             period  = 0,
                   uint32_t          latency = 0);

        /**
         * @brief Construct a connection wrapper with an existing sensor handle.
         *
         * @details
         * This constructor is used when the sensor handle is already known
         * (for example, restored from persistent state or provided externally).
         * No driver resolution is performed.
         *
         * @param handle   Existing sensor driver handle.
         * @param period   Desired sampling/update period.
         * @param latency  Maximum allowed reporting latency.
         */
        Connection(uint8_t  handle,
                   float    period  = 0,
                   uint32_t latency = 0);

        ~Connection();

        /**
         * @brief Check whether a valid sensor handle is available.
         *
         * @return true  If the internal handle is non-zero.
         * @return false If no handle has been resolved yet.
         */
        bool isValid();

        /**
         * @brief Connect to the sensor using the stored parameters.
         *
         * @details
         * Connection procedure:
         * 1. If no valid handle is present:
         *    - Send RequestDefault to resolve the default sensor driver.
         *    - Store the returned handle.
         * 2. Send RequestConnect using the resolved handle and the configured
         *    period and latency.
         * 3. Mark the connection as active if the request succeeds.
         *
         * @return true  If the connection was successfully established.
         * @return false If handle resolution or connection request failed.
         */
        bool connect();

        /**
         * @brief Update connection parameters and connect.
         *
         * @details
         * Stores the new period and latency, then delegates to @ref connect().
         *
         * Limitations:
         * - If no valid handle is present, the function returns false.
         * - If already connected, parameter updates are rejected.
         *
         * @param period   New sampling/update period.
         * @param latency  New maximum reporting latency.
         *
         * @return true  If the connection was successfully established.
         * @return false If the update or connection failed.
         */
        bool connect(float period, uint32_t latency = 0);

        /**
         * @brief Check whether the connection is currently active.
         *
         * @return true  If the connection is active.
         * @return false Otherwise.
         */
        bool isConnected();

        /**
         * @brief Disconnect from the sensor.
         *
         * @details
         * If a valid and active connection exists, a RequestDisconnect message
         * is sent to the SensorLayer. The function is safe to call multiple times.
         */
        void disconnect();
        
        /**
         * @brief Check whether the given handle matches the connected sensor.
         *
         * @param handle Sensor driver handle to compare.
         *
         * @return true  If the handles match and are valid.
         * @return false Otherwise.
         */
        bool matchesDriver(uint16_t handle);

    protected:
        /**
         * @brief Resolve the default sensor driver handle.
         *
         * @details
         * Sends a RequestDefault message to the SensorLayer in order to
         * obtain a valid driver handle for the sensor type specified
         * during construction. The resolved handle is stored internally
         * and used for subsequent connection requests.
         *
         * Failure cases:
         * - Message allocation failed.
         * - IPC request timed out or returned an error.
         *
         * @return true  If the driver handle was successfully resolved and stored.
         * @return false If the request failed or no handle was returned.
         */
        bool subscribe();

        SDK::Kernel&      mKernel;
        SDK::Sensor::Type mID;
        uint8_t           mHandle;
        float             mPeriod;
        uint32_t          mLatency;
        bool              mIsConnected;
    };

} // namespace SDK::Sensor
