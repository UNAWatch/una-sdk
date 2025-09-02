/**
 ******************************************************************************
 * @file    SensorDataParserAltimeter.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for SENSOR_TYPE_ALTIMETER
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_ALTIMETER_HPP
#define __SENSOR_DATA_PARSER_ALTIMETER_HPP

#include "SDK/SensorLayer/ISensorData.hpp"

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
             * @brief Construct a new Altimeter parser over given ISensorData
             * @param data Reference to sensor data containing 1 float value
             */
            Altimeter(const Interface::ISensorData& data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 float field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return mData.getLength() == Field::kCount;
            }

            /**
             * @brief Get altitude in meters
             * @return Altitude as float (0.0f if invalid)
             */
            float getAltitude() const
            {
                return isDataValid() ? mData.getAsFloat(Field::kAltitude) : 0.0f;
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
                kAltitude = 0, ///< Altitude in meters
                kCount         ///< Number of fields
            };

            const Interface::ISensorData& mData;
        }; /* class Altimeter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ALTIMETER_HPP */
