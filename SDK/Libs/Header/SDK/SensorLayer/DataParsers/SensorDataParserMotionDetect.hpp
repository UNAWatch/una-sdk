/**
 ******************************************************************************
 * @file    SensorDataParserAltimeter.hpp
 * @date    11-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for MOTION_DETECT sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_MOTION_DETECT_HPP
#define __SENSOR_DATA_PARSER_MOTION_DETECT_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK::SensorDataParser {

/**
 * @brief Helper class to parse Motion Detect sensor data from ISensorData.
 *
 * @details
 * Expected data layout (index-based fields):
 * - [0] uint32_t ID — motion identifier (see @ref Motion)
 *
 * The parser only reads the fields and provides type-safe accessors.
 * It does not own the underlying storage.
 */
class MotionDetect
{
public:
    /**
     * @brief Field layout indices.
     */
    enum Field : uint8_t {
        ID = 0,   ///< Motion identifier (see @ref Motion)
        COUNT     ///< Number of fields (must be last)
    };

    /**
     * @brief Motion identifier values.
     */
    enum class Motion : uint8_t {
        NO_MOTION = 0x00,  ///< No motion detected
        MOTION,            ///< Motion detected
        SIG_MOTION,        ///< Significant motion detected
        UNKNOWN            ///< Unknown / unclassified state
    };

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Reference to sensor data with 1 field: ID.
     */
    explicit MotionDetect(const Interface::ISensorData& data) : mData(&data) {}

    /**
     * @brief Construct a new parser over the given ISensorData.
     * @param data Pointer to sensor data with 1 field: ID (may be nullptr).
     */
    explicit MotionDetect(const Interface::ISensorData* data) : mData(data) {}

    /**
     * @brief Check if data is valid.
     *
     * @details
     * Validity conditions:
     *  - Non-null pointer.
     *  - Exactly 1 field present.
     *  - ID is within [NO_MOTION..UNKNOWN].
     *
     * @return true if the data passes basic structural and range checks.
     */
    bool isDataValid() const
    {
        if (mData == nullptr) {
            return false;
        }
        if (mData->getLength() != static_cast<uint8_t>(Field::COUNT)) {
            return false;
        }
        const uint32_t id = mData->getAsU32(static_cast<uint8_t>(Field::ID));
        if (id > static_cast<uint32_t>(Motion::UNKNOWN)) {
            return false;
        }
        return true;
    }

    /**
     * @brief Get the motion identifier.
     * @return Motion ID (UNKNOWN if invalid).
     */
    Motion getID() const
    {
        if (!isDataValid()) {
            return Motion::UNKNOWN;
        }
        return static_cast<Motion>(mData->getAsU32(static_cast<uint8_t>(Field::ID)));
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
     * @brief Get data timestamp in us
     * @return Data timestamp in us (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData->getTimestampUs() : 0;
    }

    /**
     * @brief Get the number of expected fields.
     * @return Always returns 1 (ID).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::COUNT);
    }

private:
    const Interface::ISensorData* mData { nullptr };
}; /* class MotionDetect */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_MOTION_DETECT_HPP */
