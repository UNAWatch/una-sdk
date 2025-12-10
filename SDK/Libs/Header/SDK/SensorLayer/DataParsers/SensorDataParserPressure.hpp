/**
 ******************************************************************************
 * @file    SensorDataParserPressure.hpp
 * @date    27-November-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Pressure sensor
 * 
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_PRESSURE_HPP
#define __SENSOR_DATA_PARSER_PRESSURE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK {
namespace SensorDataParser {

/**
 * @brief   SensorData parser for the Pressure sensor
 *
 * @details
 * The parser only reads fields and provides type-safe accessors.
 * It does not own the underlying storage.
 */
class Pressure
{
public:
    enum Field : uint8_t {
        PRESS = 0,   ///< Pressure value (units are device-specific)
        COUNT        ///< Number of fields (must be last)
    };

    /**
     * @brief   Construct parser from a reference to sensor data
     * @param   data Reference to sensor data
     */
    explicit Pressure(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief   Check if data is valid
     * @return  True if data pointer is not null and has expected field count
     */
    bool isDataValid() const
    {
        return (mData.getFieldCount() == Field::COUNT);
    }

    /**
     * @brief   Get pressure value
     * @return  Pressure in hPa. Returns -1.0f if invalid.
     */
    float getPressure() const
    {
        return isDataValid() ? mData.f[Field::PRESS] : -1.0f;
    }

    /**
     * @brief   Get timestamp of the data
     * @return  Data timestamp in ms (0 if invalid)
     */
    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0U;
    }

    /**
     * @brief   Get data timestamp in µs
     * @return  Data timestamp in µs (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData.getTimestampUs() : 0;
    }

    /**
     * @brief   Returns the number of fields
     * @return  The number of fields
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::COUNT);
    }

private:
    const SDK::Sensor::DataView mData;
}; /* class Pressure */

} /* namespace SensorDataParser */
} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_PRESSURE_HPP */
