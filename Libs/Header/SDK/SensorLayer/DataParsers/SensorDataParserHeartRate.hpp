/**
 ******************************************************************************
 * @file    SensorDataParserHeartRate.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for HEART_RATE sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_HEART_RATE_HPP
#define __SENSOR_DATA_PARSER_HEART_RATE_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse heart rate sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float heart rate (bpm)
         */
        class HeartRate
        {
        public:
            /**
             * @brief Construct a new HeartRate parser over given ISensorData
             * @param data Reference to sensor data containing 1 float field
             */
            HeartRate(const Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new HeartRate parser over given ISensorData
             * @param data Pointer to sensor data containing 1 float field
             */
            HeartRate(const Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::kCount);
            }

            /**
             * @brief Get heart rate in beats per minute (bpm)
             * @return Heart rate as float
             */
            float getBpm() const
            {
                return isDataValid() ? mData->getAsU32(Field::kBpm) : 0;
            }

            /**
             * @brief Get trust level
             * @return Trust level
             */
            float getTrustLevel() const
            {
                return isDataValid() ? mData->getAsU32(Field::kTrustLevel) : 0.f;
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
             * @brief Get number of expected fields (always 1)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::kCount;
            }

        private:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                kBpm = 0,       ///< Heart rate in bpm (float)
                kTrustLevel,    ///< Trust level (uint32_t)
                kCount          ///< Total number of fields
            };

            const Interface::ISensorData* mData;
        }; /* class HeartRate */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_HEART_RATE_HPP */
