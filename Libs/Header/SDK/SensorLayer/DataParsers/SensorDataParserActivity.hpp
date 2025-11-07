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

#include "SDK/Interfaces/ISensorData.hpp"

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
    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Reference to sensor data with 1 field: DURATION.
     */
    explicit Activity(const Interface::ISensorData& data) : mData(&data) {}

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Pointer to sensor data with 1 field: DURATION (may be nullptr).
     */
    explicit Activity(const Interface::ISensorData* data) : mData(data) {}

    /**
     * @brief Check if data is valid.
     *
     * @details
     * Validity conditions:
     *  - Non-null pointer.
     *  - Exactly 1 field present (DURATION).
     *
     * @return true if the data passes basic structural checks.
     */
    bool isDataValid() const
    {
        return (mData != nullptr) &&
               (mData->getLength() == static_cast<uint8_t>(Field::kCOUNT));
    }

    /**
     * @brief Get activity duration in milliseconds.
     * @return Duration in ms (0 if invalid).
     */
    uint32_t getDuration() const
    {
        if (!isDataValid()) {
            return 0U;
        }

        return mData->getAsU32(static_cast<uint8_t>(Field::kDURATION));
    }

    /**
     * @brief Get data timestamp in milliseconds.
     * @return Data timestamp in ms (0 if data pointer is null).
     */
    uint32_t getTimestamp() const
    {
        return (mData != nullptr) ? mData->getTimestamp() : 0U;
    }

    /**
     * @brief Get the number of expected fields.
     * @return Always returns 1 (DURATION).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::kCOUNT);
    }

private:
    /**
     * @brief Field layout indices.
     */
    enum Field : uint8_t {
        kDURATION = 0,  ///< Activity duration in milliseconds
        kCOUNT          ///< Number of fields (must be last)
    };

    const Interface::ISensorData* mData { nullptr };
}; /* class Activity */

} /* namespace SensorDataParser */
} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACTIVITY_HPP */

