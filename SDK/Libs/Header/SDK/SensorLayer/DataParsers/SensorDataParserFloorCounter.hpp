/**
 ******************************************************************************
 * @file    SensorDataParserFloorCounter.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for SENSOR_TYPE_FLOOR_COUNTER
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP
#define __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP

#include "SDK/SensorLayer/ISensorData.hpp"

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
             * @param data Reference to sensor data containing 1 int32_t field
             */
            FloorCounter(const Interface::ISensorData& data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return mData.getLength() == Field::kCount;
            }

            /**
             * @brief Get floor count (signed)
             * @return Floor count as int32_t (0 if invalid)
             */
            int32_t getFloorCount() const
            {
                return isDataValid() ? mData.getAsI32(Field::kFloors) : 0;
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
                kFloors = 0, ///< Signed floor counter (int32_t)
                kCount        ///< Total number of fields
            };

            const Interface::ISensorData& mData;
        }; /* class FloorCounter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_FLOOR_COUNTER_HPP */
