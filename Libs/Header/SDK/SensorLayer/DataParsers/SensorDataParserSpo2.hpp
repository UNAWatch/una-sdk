/**
 ******************************************************************************
 * @file    SensorDataParserSpo2.hpp
 * @brief   SensorData parser for SPO2 sensor
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_SPO2_HPP
#define __SENSOR_DATA_PARSER_SPO2_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse blood-oxygen saturation (SpO2) sensor data from ISensorData
         *
         * SpO2 is derived from the optical PPG path. Like HEART_RATE, it is delivered as a
         * processed scalar accompanied by a trust level rather than a raw waveform.
         *
         * Expected data layout:
         * - [0] float saturation (percent, typically [70..100])
         * - [1] float trust level
         */
        class Spo2
        {
        public:
            enum Field : uint8_t {
                SATURATION = 0, ///< Blood-oxygen saturation in percent (float)
                TRUST_LEVEL,    ///< Trust level (float)
                COUNT           ///< Total number of fields
            };

            /**
             * @brief Construct a new Spo2 parser over given ISensorData
             * @param view Reference to sensor data containing 2 fields
             */
            Spo2(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Check if data is valid (should contain exactly 2 fields)
             * @return true if valid
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get blood-oxygen saturation as a percentage
             * @return Saturation in percent as float (0.f if invalid)
             */
            float getSaturation() const
            {
                return isDataValid() ? mData.f[Field::SATURATION] : 0.f;
            }

            /**
             * @brief Get trust level
             * @return Trust level
             */
            float getTrustLevel() const
            {
                return isDataValid() ? mData.f[Field::TRUST_LEVEL] : 0.f;
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
             * @brief Get number of expected fields (always 2)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class Spo2 */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_SPO2_HPP */
