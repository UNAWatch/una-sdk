/**
 ******************************************************************************
 * @file    SensorDataParserGpsDistance.hpp
 * @date    27-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for GPS Distance sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GPS_DISTANCE_HPP
#define __SENSOR_DATA_PARSER_GPS_DISTANCE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse GPS sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float distance (m)
         *
         * Validity of each field is checked via corresponding mask bit.
         */
        class GpsDistance
        {
        public:
            enum Field : uint8_t {
                DISTANCE = 0,   ///< Distance, m(float)
                COUNT           ///< Total number of fields
            };

            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Reference to sensor data containing GPS fields
             */
            GpsDistance(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if datais valid
             * @return true if data length is Field::COUNT
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get distance
             * @return Distance in meters
             */
            float getDistance() const
            {
                return isDataValid() ? mData.f[Field::DISTANCE] : 0.0f;
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
             * @brief Get total number of expected fields
             * @return Field count (6)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class GpsDistance */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GPS_DISTANCE_HPP */
