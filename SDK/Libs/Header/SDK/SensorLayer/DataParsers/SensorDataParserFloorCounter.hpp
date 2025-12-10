/**
 ******************************************************************************
 * @file    SensorDataParserFloorCounter.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for FLOOR_COUNTER sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP
#define __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse floor counter sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] int32_t floor count (positive = up, negative = down)
         */
        class FloorCounter
        {
        public:
            enum Field : uint8_t {
                FLOORS_UP = 0,  ///< Signed floorsUp counter (int32_t)
                FLOORS_DOWN,    ///< Signed floorsDown counter (int32_t)
                COUNT          ///< Total number of fields
            };

            /**
             * @brief Construct a new FloorCounter parser over given ISensorData
             * @param data Reference to sensor data containing 2 int32_t field
             */
            FloorCounter(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get floorsUp count (signed)
             * @return FloorsUp count as int32_t (0 if invalid)
             */
            int32_t getFloorsUp() const
            {
                return isDataValid() ? mData.i[Field::FLOORS_UP] : 0;
            }

            /**
             * @brief Get floorsDown count (signed)
             * @return Floors down count as int32_t (0 if invalid)
             */
            int32_t getFloorsDown() const
            {
                return isDataValid() ? mData.i[Field::FLOORS_DOWN] : 0;
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
        }; /* class FloorCounter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP */
