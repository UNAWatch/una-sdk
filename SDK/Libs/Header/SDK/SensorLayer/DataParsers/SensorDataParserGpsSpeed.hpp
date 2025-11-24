/**
 ******************************************************************************
 * @file    SensorDataParserGpsSpeed.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for GPS Speed sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GPS_SPEED_HPP
#define __SENSOR_DATA_PARSER_GPS_SPEED_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse GPS sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float speed (m/s)
         *
         * Validity of each field is checked via corresponding mask bit.
         */
        class GpsSpeed
        {
        public:
            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Reference to sensor data containing GPS fields
             */
            GpsSpeed(const SDK::Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Pointer to sensor data containing GPS fields
             */
            GpsSpeed(const SDK::Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if datais valid
             * @return true if data length is Field::COUNT
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::COUNT);
            }

            /**
             * @brief Get GPS speed
             * @return Speed in meters per second
             */
            float getSpeed() const
            {
                return isDataValid() ? mData->getAsFloat(Field::SPEED) : 0.0f;
            }

            /**
             * @brief Get data timestamp in ms
             * @return Data timestamp in ms (0 if invalid)
             */
            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData->getTimestamp() : 0;
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
             * @brief Get total number of expected fields
             * @return Field count (6)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                SPEED = 0,  ///< Speed, m/s (float)
                COUNT       ///< Total number of fields
            };

        private:
            const SDK::Interface::ISensorData* mData;
        }; /* class GpsSpeed */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GPS_SPEED_HPP */
