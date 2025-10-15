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

#include "SDK/SensorLayer/SensorDriverConnection.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

namespace SDK::Sensor {

/**
 * @brief Construct a connection wrapper for a sensor driver.
 *
 * Resolves the underlying sensor driver via
 * `kernel->sensorManager.getDefaultSensor(id)` and stores connection
 * parameters for subsequent @ref connect() calls.
 *
 * @param id       Sensor type identifier to resolve the default driver.
 * @param listener Pointer to a data listener (must remain valid while connected).
 * @param period   Desired sampling/update period (units as defined by the driver; commonly seconds).
 * @param latency  Maximum report latency/batching tolerance (units as defined by the driver; commonly milliseconds).
 * @pre `kernel` must be initialized.
 * @note Use @ref isValid() to check that an underlying driver was found.
 */
DriverConnection::DriverConnection(SDK::Sensor::Type                    id,
                                   SDK::Interface::ISensorDataListener* listener,
                                   float                                period,
                                   uint32_t                             latency)
    : mDriver(SDK::KernelProviderService::GetInstance().getKernel().sensorManager.getDefaultSensor(id))
    , mListener(listener)
    , mPeriod(period)
    , mLatency(latency)
    , mUserApp(&SDK::KernelProviderService::GetInstance().getKernel().app)
{}

/**
 * @brief Check whether the underlying driver has been resolved.
 * @return `true` if a driver pointer is available, otherwise `false`.
 */
bool DriverConnection::isValid()
{
    return (mDriver != nullptr);
}

/**
 * @brief Connect to the driver using the stored period and latency.
 *
 * Delegates to the driver's `connect(listener, userApp, period, latency)`.
 *
 * @return `true` on successful connection, `false` if no driver is available or the driver rejects the request.
 * @see connect(float,uint32_t)
 */
bool DriverConnection::connect()
{
    if (!isValid()) {
        return false;
    }

    return mDriver->connect(mListener, mUserApp, mPeriod, mLatency);
}

/**
 * @brief Update period/latency and connect to the driver.
 *
 * Stores the provided parameters and then calls the parameterless @ref connect().
 *
 * @param period   Desired sampling/update period (units as defined by the driver).
 * @param latency  Maximum report latency/batching tolerance (units as defined by the driver).
 * @return `true` on successful connection, otherwise `false`.
 */
bool DriverConnection::connect(float period, uint32_t latency)
{
    if (!isValid()) {
        return false;
    }

    mPeriod  = period;
    mLatency = latency;

    return connect();
}

/**
 * @brief Disconnect the listener from the underlying driver.
 *
 * Safe to call even if the connection is not valid or already disconnected.
 * If no driver is available, the call is a no-op.
 */
void DriverConnection::disconnect()
{
    if (!isValid()) {
        return;
    }

    mDriver->disconnect(mListener);
}
        
} // namespace SDK::Sensors
