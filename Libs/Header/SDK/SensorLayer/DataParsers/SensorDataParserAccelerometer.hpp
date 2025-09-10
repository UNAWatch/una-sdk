/**
 ******************************************************************************
 * @file    SensorDataParserAccelerometer.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for ACCELEROMETER sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ACCELEROMETER_HPP
#define __SENSOR_DATA_PARSER_ACCELEROMETER_HPP

#include "SDK/Interfaces/ISensorData.hpp"

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
        class Accelerometer
        {
        public:
            /**
             * @brief Construct a new Accelerometer parser over given ISensorData
             * @param data Reference to sensor data containing 1 float value
             */
            Accelerometer(const Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new Accelerometer parser over given ISensorData
             * @param data Pointer to sensor data containing 1 float value
             */
            Accelerometer(const Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 float field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::COUNT);
            }

            /**
             * @brief Get acceleration on X axis
             * @return Acceleration in g (if data is valid), otherwise 0.0f
             */
            float getX() const
            {
                return isDataValid() ? mData->getAsFloat(Field::X) : 0.0f;
            }

            /**
             * @brief Get acceleration on Y axis
             * @return Acceleration in g (if data is valid), otherwise 0.0f
             */
            float getY() const
            {
                return isDataValid() ? mData->getAsFloat(Field::Y) : 0.0f;
            }

            /**
             * @brief Get acceleration on Z axis
             * @return Acceleration in g (if data is valid), otherwise 0.0f
             */
            float getZ() const
            {
                return isDataValid() ? mData->getAsFloat(Field::Z) : 0.0f;
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
                return Field::COUNT;
            }

        private:
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
             * @brief Reference to sensor data storage
             */
            const Interface::ISensorData* mData;
        }; /* class Accelerometer */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ACCELEROMETER_HPP */
