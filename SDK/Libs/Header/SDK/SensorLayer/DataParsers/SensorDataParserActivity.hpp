/**
 ******************************************************************************
 * @file    SensorDataParserAltimeter.hpp
 * @date    11-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for ACTIVITY sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ACTIVITY_HPP
#define __SENSOR_DATA_PARSER_ACTIVITY_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK {
namespace SensorDataParser {

/**
 * @brief Helper class to parse ACTIVITY sensor data from ISensorData.
 *
 * @details
 * Expected data layout (index-based fields):
 * - [0] uint32_t DURATION — activity duration in milliseconds.
 *
 * The parser only reads the fields and provides type-safe accessors.
 * It does not own the underlying storage.
 */
class Activity
{
public:
    enum Field : uint8_t {
        DURATION = 0,  ///< Activity duration in milliseconds
        COUNT          ///< Number of fields (must be last)
    };

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Reference to sensor data with 1 field: DURATION.
     */
    explicit Activity(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief Check if data is valid.
     *
     * @details
     * Validity conditions:
     *  - Non-null pointer.
     *  - Only needed fields present.
     *
     * @return true if the data passes basic structural checks.
     */
    bool isDataValid() const
    {
        return (mData.getFieldCount() == Field::COUNT);
    }

    /**
     * @brief Get activity duration in milliseconds.
     * @return Duration in ms (0 if invalid).
     */
    uint32_t getDuration() const
    {
        return isDataValid() ? mData.u[Field::DURATION] : 0U;
    }

    /**
     * @brief Get data timestamp in milliseconds.
     * @return Data timestamp in ms (0 if data pointer is null).
     */
    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0U;
    }

    /**
     * @brief Get data timestamp in us
     * @return Data timestamp in us (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData.getTimestampUs() : 0;
    }

    /**
     * @brief Get the number of expected fields.
     * @return Always returns 1 (DURATION).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
}; /* class Activity */

} /* namespace SensorDataParser */
} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACTIVITY_HPP */

