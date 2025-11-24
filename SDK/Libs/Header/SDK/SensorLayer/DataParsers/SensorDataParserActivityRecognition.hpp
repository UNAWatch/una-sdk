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

#include "SDK/Interfaces/ISensorData.hpp"

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
        kID = 0,        ///< Activity identifier (see @ref Activity)
        kCONFIDENCE,    ///< Confidence in percent [0..100]
        kCOUNT          ///< Number of fields (must be last)
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
    explicit ActivityRecognition(const Interface::ISensorData& data) : mData(&data) {}

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Pointer to sensor data with 2 fields: ID, CONFIDENCE (may be nullptr).
     */
    explicit ActivityRecognition(const Interface::ISensorData* data) : mData(data) {}

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
        if (mData == nullptr) {
            return false;
        }

        // Expect exactly 2 fields
        if (mData->getLength() != Field::kCOUNT) {
            return false;
        }

        const uint32_t id = mData->getAsU32(Field::kID);
        if (id > static_cast<uint32_t>(Activity::UNKNOWN)) {
            return false;
        }

        const uint32_t conf = mData->getAsU32(Field::kCONFIDENCE);
        if (conf > 100U) {
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
        if (!isDataValid()) {
            return Activity::UNKNOWN;
        }
        return static_cast<Activity>(mData->getAsU32(Field::kID));
    }

    /**
     * @brief Get the confidence in percent.
     * @return Confidence value in [0..100] (0 if invalid).
     */
    uint8_t getConfidence() const
    {
        if (!isDataValid()) {
            return 0U;
        }
        return static_cast<uint8_t>(mData->getAsU32(Field::kCONFIDENCE));
    }

    /**
     * @brief Get data timestamp in milliseconds.
     * @return Data timestamp in ms (0 if invalid).
     */
    uint32_t getTimestamp() const
    {
        return (mData != nullptr) ? mData->getTimestamp() : 0U;
    }

    /**
     * @brief Get data timestamp in us
     * @return Data timestamp in us (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData->getTimestampUs() : 0;
    }

    /**
     * @brief Get the number of expected fields.
     * @return Always returns 2 (ID, CONFIDENCE).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::kCOUNT);
    }

private:
    const Interface::ISensorData* mData { nullptr };
}; /* class ActivityRecognition */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_ACTIVITY_RECOGNITION_HPP */
