#define LOG_MODULE_PRX      "Sensor.Listener"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Simulator/Components/SensorListener.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Simulator/Components/InstanceSensorLayer.hpp"

#include <cstring>
#include <cstddef>

namespace App
{

SensorListener::SensorListener(SDK::App::DualAppComm& comm, bool isService)
    : mComm(comm)
    , mIsService(isService)
    //, mMaxPoolSize(mComm.getMsgManager().getPool().getMaxBlockSize())
{}

void SensorListener::onSdlNewData(uint16_t                 handle,
                                  const SDK::Sensor::Data* data,
                                  uint16_t                 count,
                                  uint16_t                 stride)
{
    const uint8_t* dataPtr  = reinterpret_cast<const uint8_t*>(data);
    const uint32_t baseSize = sizeof(SDK::Message::Sensor::EventData) - sizeof(SDK::Sensor::Data);

    uint32_t pid = mIsService ? mComm.getServicePID() : mComm.getGuiPID();

#if LOG_MODULE_LEVEL == LOG_LEVEL_DEBUG
    if (Sensor::Driver* d = Sensor::Manager::getInstance().getDriverByHandle(handle)) {
        LOG_DEBUG("Sensor data. PID 0x%08X. Handle %s\n", pid, d->getDescription());
    } else {
        LOG_DEBUG("Sensor data. PID 0x%08X. Handle %u.\n", pid, handle);
    }
#endif

    while (count) {
        const uint32_t size = count * stride;

        uint32_t packageSize = baseSize + size;

        //if (packageSize > mMaxPoolSize) {
        //    packageSize = mMaxPoolSize;
        //}

        const uint32_t payload = packageSize - baseSize;

        uint32_t strideCount = payload / stride;
        if (strideCount == 0) {
            LOG_ERROR("strideCount == 0. Handle %u. PID 0x%08X\n", handle, pid);
            return;
        }

        packageSize = baseSize + strideCount * stride;

        uint8_t* pool = static_cast<uint8_t*>(mComm.getMsgManager().allocateRawMemory(packageSize));
        if (!pool) {
            LOG_ERROR("Failed to allocate memory for sample. Handle %u. PID 0x%08X\n", handle, pid);
            return;
        }

        auto* event = new (pool) SDK::Message::Sensor::EventData();

        event->handle = handle;
        event->count  = strideCount;
        event->stride = stride;
        memcpy((void*) event->data, (const void*) dataPtr, strideCount * stride);

        if (!sendMessage(event)) {
            LOG_ERROR("Failed to send data. Handle %u. PID 0x%08X\n", handle, pid);
        }

        mComm.getMsgManager().releaseMessage(event);

        count   -= strideCount;
        dataPtr += strideCount * stride;
    }
}

bool SensorListener::sendMessage(SDK::MessageBase* msg)
{
    if (mIsService) {
        return mComm.sendToService(msg);
    } else {
        return mComm.sendToGui(msg);
    }
}

}
