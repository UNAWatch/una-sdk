/**
 ******************************************************************************
 * @file    SensorDataSample.hpp
 * @date    09-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data sample class with abbility to extend the cells count
 *          and access to the data fields.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_SAMPLE_HPP
#define __SENSOR_DATA_SAMPLE_HPP

#include "SDK/SensorLayer/SensorData.hpp"

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cstring>

namespace Sensor
{

struct DataSample
{
    struct FloatWriter {
        SDK::Sensor::Data& data;
        uint16_t           fieldCount;

        float& operator[](uint16_t idx) const noexcept;
    };

    struct U32Writer {
        SDK::Sensor::Data& data;
        uint16_t           fieldCount;

        uint32_t& operator[](uint16_t idx) const noexcept;
    };

    struct I32Writer {
        SDK::Sensor::Data& data;
        uint16_t           fieldCount;

        int32_t& operator[](uint16_t idx) const noexcept;
    };

    explicit DataSample(uint16_t fieldCount);

    ~DataSample();

    DataSample(const DataSample&)            = delete;
    DataSample& operator=(const DataSample&) = delete;

    DataSample(DataSample&& other)            = delete;
    DataSample& operator=(DataSample&& other) = delete;

    SDK::Sensor::Data& getData() noexcept;

    const SDK::Sensor::Data& getData() const noexcept;

    std::size_t getFieldCount() const noexcept;

    void setTimestamp(uint32_t ms) const noexcept;
    void setTimestampUs(uint64_t usTotal) const noexcept;

private:
    SDK::Sensor::Data* mData;
    std::size_t        mFieldCount;

public:
    // sub-writers
    FloatWriter f;
    U32Writer   u;
    I32Writer   i;

private:
    static SDK::Sensor::Data* createData(std::size_t fieldCount);
};

} /* namespace SDK::Sensor */

#endif /* __SENSOR_DATA_SAMPLE_HPP */
