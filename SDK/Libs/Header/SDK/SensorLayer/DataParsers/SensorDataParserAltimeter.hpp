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

#include "SDK/SensorLayer/SensorDataView.hpp"

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
                ALTITUDE = 0, ///< Altitude in meters
                COUNT         ///< Number of fields
            };

            /**
             * @brief Construct a new Altimeter parser over given ISensorData
             * @param data Reference to sensor data containing 1 float value
             */
            Altimeter(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 float field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get altitude in meters
             * @return Altitude as float (0.0f if invalid)
             */
            float getAltitude() const
            {
                return isDataValid() ? mData.f[Field::ALTITUDE] : 0.0f;
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
        }; /* class Altimeter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_ALTIMETER_HPP */
