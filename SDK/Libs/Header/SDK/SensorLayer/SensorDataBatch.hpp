/**
 ******************************************************************************
 * @file    SensorData.hpp
 * @date    28-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_BATCH_HPP
#define __SENSOR_DATA_BATCH_HPP

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"

#include <assert.h>

namespace SDK::Sensor
{
    class DataBatch
    {
    public:
        DataBatch(const SDK::Sensor::Data* base,
                  uint16_t                 count,
                  uint16_t                 stride)
            : mBase(base)
            , mCount(count)
            , mStride(stride)
            , mFieldCount(calcFieldCount(stride))
        {}

        uint16_t size() const noexcept
        {
            return mCount;
        }

        SDK::Sensor::DataView operator[](uint16_t idx) const noexcept
        {
            assert(idx < mCount);

            const auto* ptr = reinterpret_cast<const uint8_t*>(mBase) + idx * mStride;
            const auto* raw = reinterpret_cast<const SDK::Sensor::Data*>(ptr);

            return SDK::Sensor::DataView(*raw, mFieldCount);
        }

    private:
        static uint16_t calcFieldCount(uint16_t stride)
        {
            std::size_t headerSize = sizeof(SDK::Sensor::Data); // with mValues[1] inside

            assert(stride >= headerSize);

            size_t extraBytes = stride - headerSize;

            assert(extraBytes % sizeof(SDK::Sensor::Data::Field) == 0);

            size_t extraCells = extraBytes / sizeof(SDK::Sensor::Data::Field);

            return static_cast<uint16_t>(extraCells + 1);
        }

        const SDK::Sensor::Data* mBase;
        const uint16_t           mCount;
        const uint16_t           mStride;
        const uint16_t           mFieldCount;
    };

} /* namespace Sensor */

#endif /* __SENSOR_DATA_BATCH_HPP */
