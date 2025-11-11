/**
 ******************************************************************************
 * @file    SensorDataParserAltimeter.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for ALTIMETER sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ALTIMETER_HPP
#define __SENSOR_DATA_PARSER_ALTIMETER_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse altimeter sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float altitude in meters
         */
        class Altimeter
        {
        public:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                kAltitude = 0, ///< Altitude in meters
                kCount         ///< Number of fields
            };

            /**
             * @brief Construct a new Altimeter parser over given ISensorData
             * @param data Reference to sensor data containing 1 float value
             */
            Altimeter(const Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new Altimeter parser over given ISensorData
             * @param data Reference to sensor data containing 1 float value
             */
            Altimeter(const Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 float field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::kCount);
            }

            /**
             * @brief Get altitude in meters
             * @return Altitude as float (0.0f if invalid)
             */
            float getAltitude() const
            {
                return isDataValid() ? mData->getAsFloat(Field::kAltitude) : 0.0f;
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
            const Interface::ISensorData* mData;
        }; /* class Altimeter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ALTIMETER_HPP */
