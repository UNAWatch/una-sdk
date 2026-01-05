/**
 ******************************************************************************
 * @file    SensorDataParserTouch.hpp
 * @date    05-January-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Touch sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_TOUCH_HPP
#define __SENSOR_DATA_PARSER_TOUCH_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{
    /**
     * @brief Helper class to parse touch sensor data from Sensor::DataView
     *
     * Expected data layout:
     * - [0] uint32_t touch flag (expected value: 0 or 1)
     */
    class Touch
    {
    public:
        enum Field : uint8_t {
            TOUCH = 0,  ///< Touch flag (expected value: 0 or 1)
            COUNT       ///< Total number of fields
        };

        /**
         * @brief Construct a new Touch parser over given DataView
         * @param data Reference to sensor data containing 1 field
         */
        Touch(const SDK::Sensor::DataView data) : mData(data) {}

        /**
         * @brief Check if data is valid (should contain exactly 1 field)
         * @return true if valid
         */
        bool isDataValid() const
        {
            return (mData.getFieldCount() == Field::COUNT) && (mData.u[Field::TOUCH] <= 1);
        }

        /**
         * @brief Check if sensor reports touch event
         * @return true if touched (false if invalid)
         */
        bool isTouched() const
        {
            return isDataValid() ? mData.u[Field::TOUCH] : false;
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
    }; /* class Touch */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_TOUCH_HPP */
