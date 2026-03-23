/**
 ******************************************************************************
 * @file    SensorDataParserAccelerometer.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data parser for ACCELEROMETER sensor
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ACCELEROMETER_HPP
#define __SENSOR_DATA_PARSER_ACCELEROMETER_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class for parsing accelerometer sensor data from DataView
         *
         * Expected data layout:
         * - [0] Acceleration on X axis
         * - [1] Acceleration on Y axis
         * - [2] Acceleration on Z axis
         */
        class Accelerometer
        {
        public:
            /**
             * @brief Indices of accelerometer data fields
             */
            enum Field : uint8_t {
                X = 0, ///< X axis
                Y,     ///< Y axis
                Z,     ///< Z axis
                COUNT  ///< Total number of fields
            };

            /**
             * @brief Construct a new Accelerometer parser over the given sensor data
             * @param data Sensor data view containing 3 float values
             */
            Accelerometer(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check whether the sensor data is structurally valid
             *
             * @details
             * Validity conditions:
             * - The number of fields matches the expected accelerometer layout.
             *
             * @return true if the data is valid, false otherwise
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get acceleration on X axis
             * @return Acceleration on X axis if data is valid, otherwise 0.0f
             */
            float getX() const
            {
                return isDataValid() ? mData.f[Field::X] : 0.0f;
            }

            /**
             * @brief Get acceleration on Y axis
             * @return Acceleration on Y axis if data is valid, otherwise 0.0f
             */
            float getY() const
            {
                return isDataValid() ? mData.f[Field::Y] : 0.0f;
            }

            /**
             * @brief Get acceleration on Z axis
             * @return Acceleration on Z axis if data is valid, otherwise 0.0f
             */
            float getZ() const
            {
                return isDataValid() ? mData.f[Field::Z] : 0.0f;
            }

            /**
             * @brief Get acceleration values for all three axes
             * @param x Output value for X axis
             * @param y Output value for Y axis
             * @param z Output value for Z axis
             * @return true if data is valid, false otherwise
             */
            bool getXYZ(float& x, float& y, float& z) const
            {
                if (!isDataValid()) {
                    return false;
                }

                x = mData.f[Field::X];
                y = mData.f[Field::Y];
                z = mData.f[Field::Z];

                return true;
            }

            /**
             * @brief Get data timestamp in milliseconds
             * @return Data timestamp in milliseconds, or 0 if data is invalid
             */
            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData.getTimestamp() : 0;
            }

            /**
             * @brief Get data timestamp in microseconds
             * @return Data timestamp in microseconds, or 0 if data is invalid
             */
            uint64_t getTimestampUs() const
            {
                return isDataValid() ? mData.getTimestampUs() : 0;
            }

            /**
             * @brief Get the number of expected accelerometer fields
             * @return Number of expected fields
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            /**
             * @brief Sensor data view
             */
            const SDK::Sensor::DataView mData;
        }; /* class Accelerometer */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACCELEROMETER_HPP */