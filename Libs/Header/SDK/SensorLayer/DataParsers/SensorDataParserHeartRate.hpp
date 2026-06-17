/**
 ******************************************************************************
 * @file    SensorDataParserHeartRate.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for HEART_RATE sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_HEART_RATE_HPP
#define __SENSOR_DATA_PARSER_HEART_RATE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse heart rate sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float heart rate (bpm)
         * - [1] float trust level
         * - [2] float source (optional; SOURCE, present only on newer kernels)
         */
        class HeartRate
        {
        public:
            enum Field : uint8_t {
                BPM = 0,       ///< Heart rate in bpm (float)
                TRUST_LEVEL,   ///< Trust level (float)
                COUNT          ///< Base field count (BPM + TRUST_LEVEL)
            };

            // Optional field index for the HR source, emitted by newer kernels
            // in addition to the base fields. Kept out of the COUNT sequence so
            // getFieldsNumber() (the base layout) is unchanged.
            static constexpr uint8_t SOURCE = Field::COUNT;  // index 2

            /**
             * @brief Where a heart-rate sample came from.
             * @note  Values match the kernel HR source arbiter (None/Optical/
             *        External). UNKNOWN covers older kernels that emit no source
             *        field.
             */
            enum class Source : uint8_t {
                UNKNOWN  = 0,
                OPTICAL  = 1,
                EXTERNAL = 2,
            };

            /**
             * @brief Construct a new HeartRate parser over given ISensorData
             * @param view Reference to sensor data (2 base fields, optional 3rd)
             */
            HeartRate(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Check if data is valid.
             * @return true if at least the base BPM + TRUST_LEVEL fields are
             *         present. Tolerates an optional extra source field so new
             *         apps stay compatible with older two-field firmware and
             *         vice versa.
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() >= Field::COUNT);
            }

            /**
             * @brief Get heart rate in beats per minute (bpm)
             * @return Heart rate as float
             */
            float getBpm() const
            {
                return isDataValid() ? mData.f[Field::BPM] : 0.f;
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
             * @brief Get the source of this reading.
             * @return OPTICAL / EXTERNAL when the kernel reports it; UNKNOWN for
             *         older two-field frames that carry no source field.
             */
            Source getSource() const
            {
                if (mData.getFieldCount() > SOURCE) {
                    return static_cast<Source>(
                            static_cast<uint8_t>(mData.f[SOURCE]));
                }
                return Source::UNKNOWN;
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
        }; /* class HeartRate */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_HEART_RATE_HPP */
