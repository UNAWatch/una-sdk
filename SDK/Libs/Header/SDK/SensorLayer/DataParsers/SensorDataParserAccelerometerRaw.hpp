/**
 ******************************************************************************
 * @file    SensorDataParserAccelerometerRaw.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for ACCELEROMETER_RAW sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ACCELEROMETER_RAW_HPP
#define __SENSOR_DATA_PARSER_ACCELEROMETER_RAW_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse accelerometer sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float altitude in meters
         */
        class AccelerometerRaw
        {
        public:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                X = 0, ///< Axis X
                Y,     ///< Axis Y
                Z,     ///< Axis Z
                COUNT  ///< Number of fields
            };

            /**
             * @brief Construct a new Accelerometer parser over given ISensorData
             * @param data Reference to sensor data containing 1 int16 value
             */
            AccelerometerRaw(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if data is valid.
             *
             * @details
             * Validity conditions:
             *  - Non-null pointer.
             *  - Only needed fields present.
             *
             * @return true if the data passes basic structural checks.
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }
            
            /**
             * @brief Get acceleration on X axis
             * @return Acceleration in g (if data is valid), otherwise 0
             */
            int16_t getX() const
            {
                return isDataValid() ? mData.i[Field::X] : 0;
            }

            /**
             * @brief Get acceleration on Y axis
             * @return Acceleration in g (if data is valid), otherwise 0
             */
            int16_t getY() const
            {
                return isDataValid() ? mData.i[Field::Y] : 0;
            }

            /**
             * @brief Get acceleration on Z axis
             * @return Acceleration in g (if data is valid), otherwise 0
             */
            int16_t getZ() const
            {
                return isDataValid() ? mData.i[Field::Z] : 0;
            }

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
            /**
             * @brief Reference to sensor data storage
             */
            const SDK::Sensor::DataView mData;
        }; /* class Accelerometer */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACCELEROMETER_HPP */
