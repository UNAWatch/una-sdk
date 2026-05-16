/**
 ******************************************************************************
 * @file    SensorDataParserActivity.hpp
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

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse ACTIVITY sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] uint32_t activity duration in minutes
         */
        class Activity
        {
        public:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                DURATION = 0, ///< Activity duration in minutes (uint32_t)
                COUNT         ///< Total number of fields
            };

            /**
             * @brief Construct a new Activity parser over given ISensorData
             * @param data Reference to sensor data containing 1 field
             */
            Activity(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get activity duration in minutes
             * @return Duration in minutes (0 if invalid)
             */
            uint32_t getDuration() const
            {
                return isDataValid() ? mData.u[Field::DURATION] : 0;
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
        }; /* class Activity */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACTIVITY_HPP */
