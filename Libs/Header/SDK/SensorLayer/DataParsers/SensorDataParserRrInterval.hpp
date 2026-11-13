/**
 ******************************************************************************
 * @file    SensorDataParserRrInterval.hpp
 * @brief   SensorData parser for the RR_INTERVAL (beat-to-beat) sensor.
 ******************************************************************************
 *
 * Experimental: there is no firmware producer yet, so 0x44 and this layout
 * are not stable until one has been written against them.
 *
 * Opt-in. Carries the beat-to-beat R-R intervals that HRV metrics (RMSSD,
 * SDNN, DFA alpha1) are computed from, where HEART_RATE and HEART_RATE_EX
 * carry a rate. One interval per frame: a notification holding several
 * arrives as several frames, so iterate DataBatch::size().
 *
 ******************************************************************************
 */

#ifndef SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP
#define SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cmath>
#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Parse RR_INTERVAL sensor data.
         *
         * Data layout (mixed types; read [1] and [2] through the u32 view):
         * - [0] rr_ms   float  the interval in milliseconds
         * - [1] source  u32    Source, shared with HEART_RATE_EX
         * - [2] flags   u32    Flag bits; 0 claims nothing
         *
         * The frame timestamp is the instant the interval ended. A producer
         * that only sees a notification arrive back-dates each interval from
         * that arrival. Intervals are reported as measured: artefact
         * correction belongs to the consumer, ARTIFACT_SUSPECT is the
         * producer's hint.
         */
        class RrInterval
        {
        public:
            enum Field : uint8_t {
                RR_MS = 0,   ///< R-R interval, milliseconds (float)
                SOURCE,      ///< Which source produced the interval (u32, Source)
                FLAGS,       ///< Flag bits (u32)
                COUNT        ///< Field count (3)
            };

            using Source = HeartRateEx::Source;

            enum Flag : uint32_t {
                DISCONTINUITY    = 1u << 0, ///< First interval after a gap the producer knows of: stream start, reconnect, lost contact
                ARTIFACT_SUSPECT = 1u << 1, ///< The producer doubts this is a true beat-to-beat interval; the value is still reported
            };

            RrInterval(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Valid when the full layout is present (lenient so a
             *        future producer may append fields) and the interval is
             *        finite and positive; anything else is a malformed frame.
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() >= Field::COUNT) &&
                       std::isfinite(mData.f[RR_MS]) && (mData.f[RR_MS] > 0.f);
            }

            /// @return The interval in milliseconds, 0 if invalid.
            float getRrMs() const { return isDataValid() ? mData.f[RR_MS] : 0.f; }

            /**
             * @brief The source. Maps only known wire values; anything else
             *        reads as UNKNOWN.
             */
            Source getSource() const
            {
                if (!isDataValid()) {
                    return Source::UNKNOWN;
                }
                switch (mData.u[SOURCE]) {
                    case static_cast<uint32_t>(Source::OPTICAL):  return Source::OPTICAL;
                    case static_cast<uint32_t>(Source::EXTERNAL): return Source::EXTERNAL;
                    default:                                      return Source::UNKNOWN;
                }
            }

            bool hasDiscontinuity() const { return hasFlag(Flag::DISCONTINUITY); }
            bool isArtifactSuspect() const { return hasFlag(Flag::ARTIFACT_SUSPECT); }

            uint32_t getTimestamp() const   { return isDataValid() ? mData.getTimestamp() : 0; }
            uint64_t getTimestampUs() const { return isDataValid() ? mData.getTimestampUs() : 0; }

            /**
             * @brief Number of expected fields (3).
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            bool hasFlag(Flag flag) const
            {
                return isDataValid() && ((mData.u[FLAGS] & flag) != 0u);
            }

            const SDK::Sensor::DataView mData;
        }; /* class RrInterval */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP */
