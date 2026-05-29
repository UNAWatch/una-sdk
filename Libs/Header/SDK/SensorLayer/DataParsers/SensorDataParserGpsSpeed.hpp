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
         * - [0] float speed (m/s)
         * - [1] uint  speed valid (1 = current fix, 0 = acquiring / fix lost)
         * - [2] uint  dead reckoning (1 = estimated fix, speed unreliable)
         *
         * Validity of each field is checked via corresponding mask bit.
         */
        class GpsSpeed
        {
        public:
            enum Field : uint8_t {
                SPEED          = 0,  ///< Speed, m/s (float)
                SPEED_VALID    = 1,  ///< 1 when the fix is current (uint)
                DEAD_RECKONING = 2,  ///< 1 when the fix is estimated / DR (uint, §3.5)
                COUNT                ///< Total number of fields
            };

            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Reference to sensor data containing GPS fields
             */
            GpsSpeed(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if datais valid
             * @return true if data length is Field::COUNT
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get GPS speed
             * @return Speed in meters per second
             */
            float getSpeed() const
            {
                return isDataValid() ? mData.f[Field::SPEED] : 0.0f;
            }

            /**
             * @brief Whether the current speed comes from a live fix.
             * @return true if valid (false during acquisition / fix loss)
             */
            bool isSpeedValid() const
            {
                return isDataValid() && (mData.u[Field::SPEED_VALID] != 0);
            }

            /**
             * @brief Whether the receiver is dead-reckoning (estimated fix).
             * @return true if the speed is extrapolated and unreliable (§3.5)
             */
            bool isDeadReckoning() const
            {
                return isDataValid() && (mData.u[Field::DEAD_RECKONING] != 0);
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
             * @return Field count (Field::COUNT = 3: SPEED, SPEED_VALID, DEAD_RECKONING)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class GpsSpeed */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GPS_SPEED_HPP */
