/**
 ******************************************************************************
 * @file    SensorDataParserStepCounter.hpp
 * @date    09-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for STEP_DETECTOR sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_STEP_DETECTOR_HPP
#define __SENSOR_DATA_PARSER_STEP_DETECTOR_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{
    /**
     * @brief Helper class to parse step detector sensor data from ISensorData
     *
     * Expected data layout:
     * - [0] uint32_t step is detected
     */
    class StepDetector
    {
    public:
        /**
         * @brief Construct a new StepCounter parser over given ISensorData
         * @param data Reference to sensor data containing 1 field
         */
        StepDetector(const Interface::ISensorData& data) : mData(&data) {}

        /**
         * @brief Construct a new StepCounter parser over given ISensorData
         * @param data Pointer to sensor data containing 1 field
         */
        StepDetector(const Interface::ISensorData* data) : mData(data) {}

        /**
         * @brief Check if data is valid (should contain exactly 1 field)
         * @return true if valid
         */
        bool isDataValid() const
        {
            return (mData != nullptr)                    &&
                   (mData->getLength() == Field::kCount) &&
                   (mData->getAsU32(Field::kStepDetected) == 1);
        }

        /**
         * @brief Get step count
         * @return Step count as uint32_t (0 if invalid)
         */
        bool isStepDetected() const
        {
            return isDataValid();
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
        /**
         * @brief Field layout indices
         */
        enum Field : uint8_t {
            kStepDetected = 0,  ///< Step is detected (always 1)
            kCount              ///< Total number of fields
        };

        const Interface::ISensorData* mData;
    }; /* class StepCounter */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_STEP_DETECTOR_HPP */
