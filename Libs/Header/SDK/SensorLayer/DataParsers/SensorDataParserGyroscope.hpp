/**
 ******************************************************************************
 * @file    SensorDataParserAccelerometer.hpp
 * @date    23-March-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data parser for GYROSCOPE sensor
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GYROSCOPE_HPP
#define __SENSOR_DATA_PARSER_GYROSCOPE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class for parsing gyroscope sensor data from DataView
         *
         * Expected data layout:
         * - [0] Angular velocity around X axis
         * - [1] Angular velocity around Y axis
         * - [2] Angular velocity around Z axis
         */
        class Gyroscope
        {
        public:
            /**
             * @brief Indices of gyroscope data fields
             */
            enum Field : uint8_t {
                X = 0, ///< X axis
                Y,     ///< Y axis
                Z,     ///< Z axis
                COUNT  ///< Total number of fields
            };

            /**
             * @brief Construct a new Gyroscope parser over the given sensor data
             * @param data Sensor data view containing 3 float values
             */
            Gyroscope(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check whether the sensor data is structurally valid
             *
             * @details
             * Validity conditions:
             * - The number of fields matches the expected gyroscope layout.
             *
             * @return true if the data is valid, false otherwise
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get angular velocity around X axis
             * @return Angular velocity around X axis if data is valid, otherwise 0.0f
             */
            float getX() const
            {
                return isDataValid() ? mData.f[Field::X] : 0.0f;
            }

            /**
             * @brief Get angular velocity around Y axis
             * @return Angular velocity around Y axis if data is valid, otherwise 0.0f
             */
            float getY() const
            {
                return isDataValid() ? mData.f[Field::Y] : 0.0f;
            }

            /**
             * @brief Get angular velocity around Z axis
             * @return Angular velocity around Z axis if data is valid, otherwise 0.0f
             */
            float getZ() const
            {
                return isDataValid() ? mData.f[Field::Z] : 0.0f;
            }

            /**
             * @brief Get angular velocity values for all three axes
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

                x = mData.i[Field::X];
                y = mData.i[Field::Y];
                z = mData.i[Field::Z];

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
             * @brief Get the number of expected gyroscope fields
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
        }; /* class Gyroscope */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GYROSCOPE_HPP */