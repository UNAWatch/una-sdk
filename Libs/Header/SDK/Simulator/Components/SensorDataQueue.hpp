/**
 ******************************************************************************
 * @file    SensorDataQueue.hpp
 * @date    28-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Queue for SensorData
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_QUEUE_HPP
#define __SENSOR_DATA_QUEUE_HPP

#include "SDK/Simulator/Components/SensorDataHolder.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Simulator/Components/SampleRateAdapter.hpp"

#include <stdint.h>

namespace Sensor {
    class Driver;
}

namespace Sensor
{
    class DataQueue
    {
    public:
        DataQueue(Sensor::Driver&                      driver,
                  uint16_t                             fieldCount,
                  SDK::Interface::ISensorDataListener* listener,
                  float                                period,
                  uint32_t                             latency);
        ~DataQueue();

        void reinit(float period, uint32_t latency);

        void pushData(const SDK::Sensor::Data& d);
        void forceData(const SDK::Sensor::Data& d);

        SDK::Interface::ISensorDataListener* getListener() const;

        float    getPeriod() const;
        uint32_t getLatency() const;

        static uint16_t computeCapacity(float period, uint32_t latency);

    private:
        SDK::Sensor::Data& operator[](size_t i);

        void initQueue(uint16_t capacity);

        static uint16_t calcStride(uint16_t fieldCount);

        Sensor::Driver&                      mDriver;
        SDK::Interface::ISensorDataListener* mListener;
        const uint16_t                       mFieldCount;
        const uint16_t                       mStride;
        SDK::Sensor::Data*                   mQueue;
        uint16_t                             mIndex;
        uint16_t                             mSize;
        uint16_t                             mCapacity;
        float                                mPeriod;
        uint32_t                             mLatency;
        Sensor::SampleRateAdapter            mSRA;
    };

} /* namespace Sensor */

#endif /* __SENSOR_DATA_QUEUE_HPP */
