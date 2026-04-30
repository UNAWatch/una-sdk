/**
 ******************************************************************************
 * @file    SensorDataHolder.hpp
 * @date    08-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data holder class with abbility to extend the cells count
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_HOLDER_HPP
#define __SENSOR_DATA_HOLDER_HPP

#include "SDK/SensorLayer/SensorData.hpp"

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cstring>

namespace Sensor
{

struct DataHolder
{
    explicit DataHolder(std::size_t fieldCount)
        : mPtr(nullptr)
        , mFieldCount(fieldCount)
    {
        assert(fieldCount > 0);

        // Data has already contain one Cell (mValues[1]),
        // so add (fieldCount - 1) extra
        std::size_t bytes = sizeof(SDK::Sensor::Data) + (fieldCount - 1u) * sizeof(SDK::Sensor::Data::Field);

        uint8_t* mem = new uint8_t[bytes];
        std::memset(mem, 0, bytes);

        mPtr = reinterpret_cast<SDK::Sensor::Data*>(mem);
    }

    ~DataHolder()
    {
        delete [] reinterpret_cast<uint8_t*>(mPtr);
    }

    DataHolder(const DataHolder&)            = delete;
    DataHolder& operator=(const DataHolder&) = delete;

    DataHolder(DataHolder&& other)            = delete;
    DataHolder& operator=(DataHolder&& other) = delete;

    SDK::Sensor::Data& getData() noexcept
    {
        return *mPtr;
    }

    const SDK::Sensor::Data& getData() const noexcept
    {
        return *mPtr;
    }

    std::size_t getFieldCount() const noexcept
    {
        return mFieldCount;
    }

private:
    SDK::Sensor::Data* mPtr;
    std::size_t        mFieldCount;
};

} /* namespace SDK::Sensor */

#endif /* __SENSOR_DATA_HOLDER_HPP */
