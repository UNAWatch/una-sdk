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

#include "SDK/SensorLayer/SensorDataView.hpp"

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
            enum Field : uint8_t {
                BPM = 0,       ///< Heart rate in bpm (float)
                TRUST_LEVEL,    ///< Trust level (uint32_t)
                COUNT          ///< Total number of fields
            };

            /**
             * @brief Construct a new HeartRate parser over given ISensorData
             * @param view Reference to sensor data containing 1 float field
             */
            HeartRate(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get heart rate in beats per minute (bpm)
             * @return Heart rate as float
             */
            float getBpm() const
            {
                return isDataValid() ? mData.f[Field::BPM] : 0.f;
            }

            /**
             * @brief Get trust level
             * @return Trust level
             */
            float getTrustLevel() const
            {
                return isDataValid() ? mData.f[Field::TRUST_LEVEL] : 0.f;
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
        }; /* class HeartRate */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_HEART_RATE_HPP */
