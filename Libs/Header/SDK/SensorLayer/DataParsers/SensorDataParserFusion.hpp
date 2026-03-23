/**
 ******************************************************************************
 * @file    SensorDataParserFusion.hpp
 * @date    23-March-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data parser for FUSION sensor
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_FUSION_HPP
#define __SENSOR_DATA_PARSER_FUSION_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class for parsing fusion sensor data from DataView
         *
         * Expected data layout:
         * - [0] Accelerometer X axis
         * - [1] Accelerometer Y axis
         * - [2] Accelerometer Z axis
         * - [3] Gyroscope X axis
         * - [4] Gyroscope Y axis
         * - [5] Gyroscope Z axis
         */
        class Fusion
        {
        public:
            /**
             * @brief Indices of fusion sensor data fields
             */
            enum Field : uint8_t {
                ACCEL_X = 0, ///< Accelerometer X axis
                ACCEL_Y,     ///< Accelerometer Y axis
                ACCEL_Z,     ///< Accelerometer Z axis
                GYRO_X,      ///< Gyroscope X axis
                GYRO_Y,      ///< Gyroscope Y axis
                GYRO_Z,      ///< Gyroscope Z axis
                COUNT        ///< Total number of fields
            };

            struct Axes3f
            {
                float x;
                float y;
                float z;
            };

            struct Data
            {
                Axes3f accel;
                Axes3f gyro;
            };

            /**
             * @brief Construct a new Fusion parser over the given sensor data
             * @param data Sensor data view containing 6 float values
             */
            Fusion(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check whether the sensor data is structurally valid
             *
             * @details
             * Validity conditions:
             * - The number of fields matches the expected fusion sensor layout.
             *
             * @return true if the data is valid, false otherwise
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get accelerometer data
             * @param data Output accelerometer data
             * @return true if data is valid, false otherwise
             */
            bool getAccelData(Axes3f& data) const
            {
                if (!isDataValid()) {
                    return false;
                }

                data.x = mData.f[Field::ACCEL_X];
                data.y = mData.f[Field::ACCEL_Y];
                data.z = mData.f[Field::ACCEL_Z];

                return true;
            }

            /**
             * @brief Get gyroscope data
             * @param data Output gyroscope data
             * @return true if data is valid, false otherwise
             */
            bool getGyroData(Axes3f& data) const
            {
                if (!isDataValid()) {
                    return false;
                }

                data.x = mData.f[Field::GYRO_X];
                data.y = mData.f[Field::GYRO_Y];
                data.z = mData.f[Field::GYRO_Z];

                return true;
            }

            /**
             * @brief Get full fusion sensor data
             * @param data Output structure containing accelerometer and gyroscope data
             * @return true if data is valid, false otherwise
             */
            bool getData(Data& data) const
            {
                if (!isDataValid()) {
                    return false;
                }

                data.accel.x = mData.f[Field::ACCEL_X];
                data.accel.y = mData.f[Field::ACCEL_Y];
                data.accel.z = mData.f[Field::ACCEL_Z];

                data.gyro.x = mData.f[Field::GYRO_X];
                data.gyro.y = mData.f[Field::GYRO_Y];
                data.gyro.z = mData.f[Field::GYRO_Z];

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
             * @brief Get the number of expected fusion sensor fields
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
        }; /* class Fusion */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_FUSION_HPP */
