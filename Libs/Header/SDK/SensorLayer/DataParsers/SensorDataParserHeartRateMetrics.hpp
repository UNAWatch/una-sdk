/**
 ******************************************************************************
 * @file    SensorDataParserHeartRateMetrics.hpp
 * @date    15-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for HEART_RATE_METRICS sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_HEART_RATE_METRICS_HPP
#define __SENSOR_DATA_PARSER_HEART_RATE_METRICS_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse HEART_RATE_METRICS sensor data from DataView.
         *
         * @details
         * This parser provides typed access to aggregated heart rate metrics.
         * It does not own the underlying storage.
         *
         * Expected data layout (index-based fields):
         * - [0] float AHR — Average Heart Rate in beats per minute (bpm)
         * - [1] float RHR — Resting Heart Rate in beats per minute (bpm)
         */
        class HeartRateMetrics
        {
        public:
            enum Field : uint8_t {
                AHR = 0,    ///< Average Heart Rate (float, bpm)
                RHR,        ///< Resting Heart Rate (float, bpm)
                COUNT       ///< Number of fields (must be last)
            };

            /**
             * @brief Construct a new HeartRateMetrics parser over the given DataView.
             *
             * @param view Reference-like view of sensor data containing AHR and RHR fields.
             */
            HeartRateMetrics(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Check if the data contains all expected fields.
             *
             * @return true  If the data has exactly @ref Field::COUNT fields.
             * @return false Otherwise.
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() == Field::COUNT);
            }

            /**
             * @brief Get Average Heart Rate (AHR) in beats per minute (bpm).
             *
             * @return AHR value if data is valid, otherwise 0.0f.
             */
            float getAhr() const
            {
                return isDataValid() ? mData.f[Field::AHR] : 0.0f;
            }

            /**
             * @brief Get Resting Heart Rate (RHR) in beats per minute (bpm).
             *
             * @return RHR value if data is valid, otherwise 0.0f.
             */
            float getRhr() const
            {
                return isDataValid() ? mData.f[Field::RHR] : 0.0f;
            }

            /**
             * @brief Get data timestamp in milliseconds.
             *
             * @return Timestamp in ms if data is valid, otherwise 0.
             */
            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData.getTimestamp() : 0;
            }

            /**
             * @brief Get data timestamp in microseconds.
             *
             * @return Timestamp in us if data is valid, otherwise 0.
             */
            uint64_t getTimestampUs() const
            {
                return isDataValid() ? mData.getTimestampUs() : 0;
            }

            /**
             * @brief Get the number of expected fields for HEART_RATE_METRICS.
             *
             * @return Always returns @ref Field::COUNT.
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class HeartRateMetrics */
    } /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_HEART_RATE_METRICS_HPP */
