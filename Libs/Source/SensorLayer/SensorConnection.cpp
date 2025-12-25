/**
 ******************************************************************************
 * @file    SensorConnection.cpp
 * @date    17-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   High-level connection wrapper for sensor drivers.
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
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include <stdlib.h>

namespace SDK::Sensor {

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
Connection::Connection(SDK::Sensor::Type id,
                       float             period,
                       uint32_t          latency)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mID(id)
    , mHandle(0)
    , mPeriod(period)
    , mLatency(latency)
    , mIsConnected(false)
{}

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
Connection::Connection(uint8_t  handle,
                       float    period,
                       uint32_t latency)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mID(SDK::Sensor::Type::UNKNOWN)
    , mHandle(handle)
    , mPeriod(period)
    , mLatency(latency)
    , mIsConnected(false)
{}

Connection::~Connection()
{
    disconnect();
}

/**
 * @brief Check whether a valid sensor handle is available.
 *
 * @return true  If the internal handle is non-zero.
 * @return false If no handle has been resolved yet.
 */
bool Connection::isValid()
{
    return (mHandle != 0);
}

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
bool Connection::connect()
{
    if (!isValid()) {
        if (!subscribe()) {
            return false;
        }
    }

    auto reqConnect = SDK::make_msg<SDK::Message::Sensor::RequestConnect>(mKernel);
    if (!reqConnect) {
        return false;
    }

    reqConnect->handle  = mHandle;
    reqConnect->period  = mPeriod;
    reqConnect->latency = mLatency;

    if (reqConnect.send(100) || reqConnect.ok()) {
        mIsConnected = true;
    }

    return mIsConnected;
}

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
bool Connection::connect(float period, uint32_t latency)
{
    if (!isValid()) {
        if (!subscribe()) {
            return false;
        }
    }

    if (mIsConnected) {
        return false;
    }

    mPeriod  = period;
    mLatency = latency;

    return connect();
}

/**
 * @brief Check whether the connection is currently active.
 *
 * @return true  If the connection is active.
 * @return false Otherwise.
 */
bool Connection::isConnected()
{
    return mIsConnected;
}

/**
 * @brief Disconnect from the sensor.
 *
 * @details
 * If a valid and active connection exists, a RequestDisconnect message
 * is sent to the SensorLayer. The function is safe to call multiple times.
 */
void Connection::disconnect()
{
    if (!isValid()) {
        return;
    }

    if (mIsConnected) {
        auto request = SDK::make_msg<SDK::Message::Sensor::RequestDisconnect>(mKernel);
        if (request) {
            request->handle = mHandle;
            request.send();
            mIsConnected = false;
        }
    }
}

/**
 * @brief Check whether the given handle matches the connected sensor.
 *
 * @param handle Sensor driver handle to compare.
 *
 * @return true  If the handles match and are valid.
 * @return false Otherwise.
 */
bool Connection::matchesDriver(uint16_t handle)
{
    if (!isValid()) {
        return false;
    }

    if (handle == 0) {
        return false;
    }

    return mHandle == handle;
}

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
bool Connection::subscribe()
{
    auto req = SDK::make_msg<SDK::Message::Sensor::RequestDefault>(mKernel);
    if (!req) {
        return false;
    }

    req->id = mID;

    if (!req.send(100) || !req.ok()) {
        return false;
    }

    mHandle = req->handle;

    return true;
}

} // namespace SDK::Sensor
