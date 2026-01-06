/**
 ******************************************************************************
 * @file    SensorDataParserWristMotion.hpp
 * @date    06-January-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for WRIST_MOTION sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_WRIST_MOTION_HPP
#define __SENSOR_DATA_PARSER_WRIST_MOTION_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{
    /**
     * @brief Helper class to parse WRIST_MOTION sensor data from ISensorData
     *
     * Expected data layout:
     * - [0] uint32_t wrist motion event flag (1 when event is detected)
     */
    class WristMotion
    {
    public:
        enum Field : uint8_t {
            WRIST_MOTION = 0,  ///< Wrist motion event flag
            COUNT              ///< Total number of fields
        };

        /**
         * @brief Construct a new WRIST_MOTION parser over given ISensorData
         * @param data Reference to sensor data containing 1 field
         */
        WristMotion(const SDK::Sensor::DataView data) : mData(data) {}

        /**
         * @brief Check if data is valid (should contain exactly 1 field)
         * @return true if valid and wrist motion event flag is set to 1
         */
        bool isDataValid() const
        {
            return (mData.getFieldCount() == Field::COUNT) && (mData.u[Field::WRIST_MOTION] == 1);
        }

        /**
         * @brief Check if wrist motion event is detected
         * @return true if wrist motion event is detected (false if invalid)
         */
        bool isWristMotion() const
        {
            return isDataValid();
        }

        /**
         * @brief Get data timestamp in ms
         * @return Data timestamp in ms (0 if invalid)
         */
        uint32_t getTimestamp() const
        {
            return isDataValid() ? mData.getTimestamp() : 0;
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
         * @brief Get number of expected fields (always 1)
         */
        static constexpr uint8_t getFieldsNumber()
        {
            return Field::COUNT;
        }

    private:
        const SDK::Sensor::DataView mData;
    }; /* class WristMotion */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_WRIST_MOTION_HPP */
