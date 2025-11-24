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

#include "SDK/Interfaces/ISensorData.hpp"

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
            /**
             * @brief Construct a new FloorCounter parser over given ISensorData
             * @param data Reference to sensor data containing 2 int32_t field
             */
            FloorCounter(const Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new FloorCounter parser over given ISensorData
             * @param data Pointer to sensor data containing 2 int32_t field
             */
            FloorCounter(const Interface::ISensorData* data) : mData(data) {}


            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::kCount);
            }

            /**
             * @brief Get floorsUp count (signed)
             * @return FloorsUp count as int32_t (0 if invalid)
             */
            int32_t getFloorsUp() const
            {
                return isDataValid() ? mData->getAsI32(Field::kFloorsUp) : 0;
            }

            /**
             * @brief Get floorsDown count (signed)
             * @return Floors down count as int32_t (0 if invalid)
             */
            int32_t getFloorsDown() const
            {
                return isDataValid() ? mData->getAsI32(Field::kFloorsDown) : 0;
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
	     * @brief Get data timestamp in us
	     * @return Data timestamp in us (0 if invalid)
	     */
	    uint64_t getTimestampUs() const
	    {
        	return isDataValid() ? mData->getTimestampUs() : 0;
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
                kFloorsUp = 0,  ///< Signed floorsUp counter (int32_t)
                kFloorsDown,    ///< Signed floorsDown counter (int32_t)
                kCount          ///< Total number of fields
            };

            const Interface::ISensorData* mData;
        }; /* class FloorCounter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP */
