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

#ifndef __SENSOR_DATA_VIEW_HPP
#define __SENSOR_DATA_VIEW_HPP

#include "SDK/SensorLayer/SensorData.hpp"
#include <cstdint>

namespace SDK::Sensor
{
struct DataView
{
    struct FloatView {
        const SDK::Sensor::Data& data;
        uint16_t                 fieldCount;

        float operator[](uint16_t idx) const noexcept
        {
            assert(idx < fieldCount);
            return data.mValue[idx].f;
        }
    };

    struct U32View {
        const SDK::Sensor::Data& data;
        uint16_t                 fieldCount;

        uint32_t operator[](uint16_t idx) const noexcept
        {
            assert(idx < fieldCount);
            return data.mValue[idx].u32;
        }
    };

    struct I32View {
        const SDK::Sensor::Data& data;
        uint16_t                 fieldCount;

        int32_t operator[](uint16_t idx) const noexcept
        {
            assert(idx < fieldCount);
            return data.mValue[idx].i32;
        }
    };

    DataView(const SDK::Sensor::Data& data, uint16_t fieldCount)
        : f{data, fieldCount}
        , u{data, fieldCount}
        , i{data, fieldCount}
        , mData(data)
        , mFieldCount(fieldCount)
    {}

    uint32_t getTimestamp() const noexcept
    {
        return mData.mTimeStamp;
    }

    uint64_t getTimestampUs() const noexcept
    {
        return static_cast<uint64_t>(mData.mTimeStamp) * 1000ULL + mData.mTimeStampUs;
    }

    uint16_t getFieldCount() const noexcept
    {
        return mFieldCount;
    }

    // sub views
    FloatView f;
    U32View   u;
    I32View   i;

private:
    const SDK::Sensor::Data& mData;
    const uint16_t           mFieldCount;
};


} /* namespace SDK::Sensor */

#endif /* __SENSOR_DATA_VIEW_HPP */
