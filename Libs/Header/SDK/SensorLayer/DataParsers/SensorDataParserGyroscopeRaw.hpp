/**
 ******************************************************************************
 * @file    SensorDataParserGyroscopeRaw.hpp
 * @date    23-March-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data parser for GYROSCOPE_RAW sensor
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GYROSCOPE_RAW_HPP
#define __SENSOR_DATA_PARSER_GYROSCOPE_RAW_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class for parsing raw gyroscope sensor data from DataView
         *
         * Expected data layout:
         * - [0] Raw X axis value
         * - [1] Raw Y axis value
         * - [2] Raw Z axis value
         */
        class GyroscopeRaw
        {
        public:
            /**
             * @brief Indices of raw gyroscope data fields
             */
            enum Field : uint8_t {
                X = 0, ///< X axis
                Y,     ///< Y axis
                Z,     ///< Z axis
                COUNT  ///< Total number of fields
            };

            /**
             * @brief Construct a new GyroscopeRaw parser over the given sensor data
             * @param data Sensor data view containing 3 raw integer values
             */
            GyroscopeRaw(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check whether the sensor data is structurally valid
             *
             * @details
             * Validity conditions:
             * - The number of fields matches the expected raw gyroscope layout.
             *
             * @return true if the data is valid, false otherwise
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }
            
            /**
             * @brief Get raw X axis value
             * @return Raw X axis value if data is valid, otherwise 0
             */
            int16_t getX() const
            {
                return isDataValid() ? mData.i[Field::X] : 0;
            }

            /**
             * @brief Get raw Y axis value
             * @return Raw Y axis value if data is valid, otherwise 0
             */
            int16_t getY() const
            {
                return isDataValid() ? mData.i[Field::Y] : 0;
            }

            /**
             * @brief Get raw Z axis value
             * @return Raw Z axis value if data is valid, otherwise 0
             */
            int16_t getZ() const
            {
                return isDataValid() ? mData.i[Field::Z] : 0;
            }

            /**
             * @brief Get raw values for all three axes
             * @param x Output value for X axis
             * @param y Output value for Y axis
             * @param z Output value for Z axis
             * @return true if data is valid, false otherwise
             */
            bool getXYZ(int16_t& x, int16_t& y, int16_t& z) const
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
             * @brief Get the number of expected raw gyroscope fields
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
        }; /* class GyroscopeRaw */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GYROSCOPE_RAW_HPP */
