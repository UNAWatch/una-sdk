#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include <stdlib.h>

namespace SDK::Sensor {

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

bool Connection::isValid()
{
    return (mHandle != 0);
}

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

    // Connected only when the kernel actually returned SUCCESS. sendMessage()
    // returns true even when only the *response* timed out (not just on a real
    // reply), so the old `send() || ok()` latched mIsConnected on a timed-out
    // ack -- the field failure mode -- as well as on FAIL, then never retried.
    // Gating on ok() leaves mIsConnected false on timeout/FAIL so the caller
    // can retry. Mirrors subscribe()'s `!send() || !ok()`.
    mIsConnected = reqConnect.send(100) && reqConnect.ok();

    return mIsConnected;
}

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

bool Connection::isConnected()
{
    return mIsConnected;
}

void Connection::disconnect()
{
    if (!isValid()) {
        return;
    }

    // Disconnect whenever we hold a handle, not only when mIsConnected is set.
    // A connect() whose ack timed out can still have registered the listener
    // kernel-side (sendMessage returns true on a response timeout), leaving
    // mIsConnected false; gating on it would leak that listener -- and any
    // duty-cycled hardware behind it -- until app stop. Disconnecting an
    // unregistered listener is a safe kernel-side no-op.
    auto request = SDK::make_msg<SDK::Message::Sensor::RequestDisconnect>(mKernel);
    if (request) {
        request->handle = mHandle;
        request.send();
    }
    // Clear regardless of whether the message could be allocated: disconnect()
    // means this Connection no longer considers itself connected, and nothing
    // retries disconnect().
    mIsConnected = false;
}

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
