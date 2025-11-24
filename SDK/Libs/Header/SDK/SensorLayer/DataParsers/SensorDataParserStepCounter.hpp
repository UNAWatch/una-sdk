/**
 ******************************************************************************
 * @file    SensorDataParserStepCounter.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for STEP_COUNTER sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_STEP_COUNTER_HPP
#define __SENSOR_DATA_PARSER_STEP_COUNTER_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse step counter sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] uint32_t step count
         */
        class StepCounter
        {
        public:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                kStepCount = 0, ///< Step count (uint32_t)
                kCount           ///< Total number of fields
            };

            /**
             * @brief Construct a new StepCounter parser over given ISensorData
             * @param data Reference to sensor data containing 1 field
             */
            StepCounter(const Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new StepCounter parser over given ISensorData
             * @param data Pointer to sensor data containing 1 field
             */
            StepCounter(const Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if data is valid (should contain exactly 1 field)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::kCount);
            }

            /**
             * @brief Get step count
             * @return Step count as uint32_t (0 if invalid)
             */
            uint32_t getStepCount() const
            {
                return isDataValid() ? mData->getAsU32(Field::kStepCount) : 0;
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
            const Interface::ISensorData* mData;
        }; /* class StepCounter */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_STEP_COUNTER_HPP */
