/**
 ******************************************************************************
 * @file    SensorDataParserAltimeter.hpp
 * @date    11-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for ACTIVITY_RECOGNITION sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ACTIVITY_RECOGNITION_HPP
#define __SENSOR_DATA_PARSER_ACTIVITY_RECOGNITION_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser {

/**
 * @brief Helper class to parse Activity Recognition data from ISensorData.
 *
 * @details
 * Expected data layout (index-based fields):
 * - [0] uint32_t ID          — activity identifier (see @ref Activity)
 * - [1] uint32_t CONFIDENCE  — confidence in percent (0..100)
 *
 * The parser only reads the fields and provides type-safe accessors.
 * It does not own the underlying storage.
 */
class ActivityRecognition
{
public:
    /**
     * @brief Field layout indices.
     */
    enum Field : uint8_t {
        ID = 0,        ///< Activity identifier (see @ref Activity)
        CONFIDENCE,    ///< Confidence in percent [0..100]
        COUNT          ///< Number of fields (must be last)
    };

    /**
     * @brief Activity identifier values.
     */
    enum class Activity : uint8_t {
        STILL   = 0x00,  ///< No movement detected
        WALKING,         ///< Walking pattern detected
        RUNNING,         ///< Running pattern detected
        UNKNOWN          ///< Unknown / unclassified activity
    };

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Reference to sensor data with 2 fields: ID, CONFIDENCE.
     */
    explicit ActivityRecognition(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief Check if data is valid.
     *
     * @details
     * Validity conditions:
     *  - Non-null pointer.
     *  - Exactly 2 fields present.
     *  - ID is within [STILL..UNKNOWN].
     *  - CONFIDENCE is within [0..100].
     *
     * @return true if the data passes basic structural and range checks.
     */
    bool isDataValid() const
    {
        // Expect exactly 2 fields
        if (mData.getFieldCount() != Field::COUNT) {
            return false;
        }

        if (mData.u[Field::ID] > static_cast<uint32_t>(Activity::UNKNOWN)) {
            return false;
        }

        if (mData.u[Field::CONFIDENCE] > 100U) {
            return false;
        }

        return true;
    }

    /**
     * @brief Get the activity identifier.
     * @return Activity ID (UNKNOWN if invalid).
     */
    Activity getID() const
    {
        return isDataValid() ? static_cast<Activity>(mData.u[Field::ID]) : Activity::UNKNOWN;
    }

    /**
     * @brief Get the confidence in percent.
     * @return Confidence value in [0..100] (0 if invalid).
     */
    uint8_t getConfidence() const
    {
        return isDataValid() ? static_cast<uint8_t>(mData.u[Field::CONFIDENCE]) : 0;
    }

    /**
     * @brief Get data timestamp in milliseconds.
     * @return Data timestamp in ms (0 if invalid).
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
     * @return Always returns 2 (ID, CONFIDENCE).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
}; /* class ActivityRecognition */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_ACTIVITY_RECOGNITION_HPP */
