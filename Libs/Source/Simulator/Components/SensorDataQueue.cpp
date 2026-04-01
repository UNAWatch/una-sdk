/**
 ******************************************************************************
 * @file    SensorDataQueue.cpp
 * @date    28-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Queue for SensorData
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Components/SensorDataQueue.hpp"
#include "SDK/Simulator/Components/SensorDriver.hpp"

#include <assert.h>

#define LOG_MODULE_PRX      "SensorDataQueue"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Sensor::DataQueue::DataQueue(Sensor::Driver&                      driver,
                             uint16_t                             fieldCount,
                             SDK::Interface::ISensorDataListener* listener,
                             float                                period,
                             uint32_t                             latency)
    : mDriver(driver)
    , mListener(listener)
    , mFieldCount(fieldCount)
    , mStride(calcStride(fieldCount))
    , mQueue(nullptr)
    , mIndex(0)
    , mSize(0)
    , mCapacity(0)
    , mPeriod(period)
    , mLatency(latency)
    , mSRA(static_cast<uint64_t>(period) * 1000)
{
    initQueue( computeCapacity(mPeriod, latency) );
}

/**
 * @brief Destroy the DataQueue.
 *
 * @details
 * Currently does nothing explicit. The internal vectors (mQueue, mQueueUser)
 * free themselves.
 */
Sensor::DataQueue::~DataQueue()
{
    ::operator delete(mQueue, std::align_val_t(4));
//    delete [] ((uint8_t*)mQueue);
}

uint16_t Sensor::DataQueue::computeCapacity(float period, uint32_t latency)
{
    if (period <= 0.0f) {
        return 1;
    }

    uint16_t capacity = static_cast<uint16_t>(latency / period);

    if (capacity == 0) {
        capacity = 1;
    }

    if (capacity > 500) {
        capacity = 500;
    }

    return capacity;
}

void Sensor::DataQueue::initQueue(uint16_t capacity)
{
    mCapacity = capacity;
    mSize     = mCapacity * mStride;

//    uint8_t* mem = new uint8_t[mSize];
    void* mem = ::operator new(mSize, std::align_val_t(4)); // aligned

    if (!mem) {
        return;
    }

    std::memset(mem, 0, mSize);

    mQueue = reinterpret_cast<SDK::Sensor::Data*>(mem);

    mIndex = 0;
}

void Sensor::DataQueue::reinit(float period, uint32_t latency)
{
    // No changes? Exit.
    if (mPeriod == period && mLatency == latency) {
        return;
    }

    mPeriod  = period;
    mLatency = latency;

    mSRA.setExpectedPeriod(static_cast<uint64_t>(mPeriod) * 1000);
    mSRA.reset();

    uint32_t newCapacity = computeCapacity(mPeriod, mLatency);

    // No changes? Exit.
    if (newCapacity == mCapacity) {
        return;
    }

    const uint8_t* oldQueue    = (uint8_t*)mQueue;
    const uint16_t oldCapacity = mCapacity;

    // Create a new queue
    initQueue(newCapacity);

    for (uint16_t idx = 0; idx < oldCapacity; ++idx) {
        SDK::Sensor::Data* item = (SDK::Sensor::Data*) &oldQueue[idx * mStride];
        forceData(*item);
    }

    ::operator delete((void*)oldQueue, std::align_val_t(4));
//    delete [] oldQueue;
}

/**
 * @brief Push one new Sensor::Data sample into this queue (rate-limited).
 *
 * @param src Incoming sample from the sensor/driver.
 *
 * @details
 * This does NOT always emit to the listener.
 *
 * Logic:
 * - Ask mSRA.shouldEmit().
 * - If it returns true, we forward the sample to forceData().
 * - If it returns false, we skip (listener doesn't need every single sample).
 *
 * This is how per-listener downsampling / decimation is implemented.
 */
void Sensor::DataQueue::pushData(const SDK::Sensor::Data& d)
{
    if (mSRA.shouldEmit(d.mTimeStamp * 1000 + d.mTimeStampUs)) {
        forceData(d);
    }
}

void Sensor::DataQueue::forceData(const SDK::Sensor::Data& d)
{
    std::uint8_t* base = reinterpret_cast<std::uint8_t*>(mQueue);
    std::uint8_t* dst  = base + static_cast<std::size_t>(mIndex) * mStride;
    std::memcpy(dst, &d, mStride);

    if (++mIndex >= mCapacity) {
        mIndex = 0;

        if (mListener) {
            mListener->onSdlNewData(mDriver.getHandle(),
                                    mQueue,
                                    mCapacity,
                                    mStride);
        }
    }
}

/**
 * @brief Return the listener associated with this queue.
 *
 * @return Pointer to the ISensorDataListener, or nullptr if none.
 */
SDK::Interface::ISensorDataListener* Sensor::DataQueue::getListener() const
{
    return mListener;
}

/**
 * @brief Get the currently configured sampling period for this listener.
 *
 * @return Period in ms.
 */
float Sensor::DataQueue::getPeriod() const
{
    return mPeriod;
}

uint32_t Sensor::DataQueue::getLatency() const
{
    return mLatency;
}

/**
 * @brief Indexed access to the internal ring buffer.
 *
 * @param i Index into mQueue.
 *
 * @return Reference to the i-th Sensor::Data entry.
 *
 * @details
 * This is mainly useful for debugging or inspection. Normal data delivery
 * to listeners uses mQueueUser (pointers to all elements), not manual indexing.
 */
SDK::Sensor::Data& Sensor::DataQueue::operator[](size_t i)
{
    assert(i < mCapacity);

    std::uint8_t* base = reinterpret_cast<std::uint8_t*>(mQueue);
    auto* ptr          = reinterpret_cast<SDK::Sensor::Data*>(base + i * mStride);

    return *ptr;
}

uint16_t Sensor::DataQueue::calcStride(uint16_t fieldCount)
{
    assert(fieldCount > 0);

    const std::size_t headerSize  = sizeof(SDK::Sensor::Data); // with mValue[1] inside
    const std::size_t extraFields = (fieldCount - 1) * sizeof(SDK::Sensor::Data::Field);

    return static_cast<uint16_t>(headerSize + extraFields);
}
