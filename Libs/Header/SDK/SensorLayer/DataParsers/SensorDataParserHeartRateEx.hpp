/**
 ******************************************************************************
 * @file    SensorDataParserHeartRateEx.hpp
 * @brief   SensorData parser for the HEART_RATE_EX (multi-source) sensor.
 ******************************************************************************
 *
 * Opt-in companion to HEART_RATE. Carries the kernel's arbitrated reading (same
 * value as HEART_RATE) plus the source that was chosen and the raw per-source
 * readings, so an app can show source-driven status and log internal (PPG),
 * external (BLE strap), and arbitrated HR as separate FIT series.
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_HEART_RATE_EX_HPP
#define __SENSOR_DATA_PARSER_HEART_RATE_EX_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Parse HEART_RATE_EX sensor data.
         *
         * Data layout (all float):
         * - [0] arbitrated bpm        (the kernel's chosen value; == HEART_RATE[0])
         * - [1] arbitrated trust
         * - [2] source                (Source: 0 none, 1 optical, 2 external)
         * - [3] optical  bpm          (raw PPG; 0 when absent/stale/off-wrist)
         * - [4] optical  trust
         * - [5] external bpm          (raw strap; 0 when absent/stale)
         * - [6] external trust
         */
        class HeartRateEx
        {
        public:
            enum Field : uint8_t {
                BPM = 0,        ///< Arbitrated heart rate (bpm)
                TRUST_LEVEL,    ///< Arbitrated trust level
                SOURCE,         ///< Which source was chosen (Source)
                OPTICAL_BPM,    ///< Raw optical (PPG) bpm (0 if none)
                OPTICAL_TRUST,  ///< Raw optical trust
                EXTERNAL_BPM,   ///< Raw external (strap) bpm (0 if none)
                EXTERNAL_TRUST, ///< Raw external trust
                COUNT           ///< Field count (7)
            };

            /**
             * @brief Which source produced the arbitrated reading.
             * @note  Values are the on-wire contract shared with the kernel HR
             *        source arbiter (Sensor::HrSource). The kernel locks the two
             *        enums together with static_asserts.
             */
            enum class Source : uint8_t {
                UNKNOWN  = 0,   ///< none / no valid HR
                OPTICAL  = 1,   ///< wrist optical (PPG)
                EXTERNAL = 2,   ///< external BLE strap
            };

            HeartRateEx(const SDK::Sensor::DataView view) : mData(view) {}

            /**
             * @brief Valid when at least the full EX layout is present (lenient
             *        so a future kernel may append fields without breaking apps).
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() >= Field::COUNT);
            }

            float getBpm() const          { return isDataValid() ? mData.f[BPM] : 0.f; }
            float getTrustLevel() const   { return isDataValid() ? mData.f[TRUST_LEVEL] : 0.f; }

            float getOpticalBpm() const   { return isDataValid() ? mData.f[OPTICAL_BPM] : 0.f; }
            float getOpticalTrust() const { return isDataValid() ? mData.f[OPTICAL_TRUST] : 0.f; }

            float getExternalBpm() const  { return isDataValid() ? mData.f[EXTERNAL_BPM] : 0.f; }
            float getExternalTrust() const{ return isDataValid() ? mData.f[EXTERNAL_TRUST] : 0.f; }

            /**
             * @brief The chosen source. Maps only known wire values; anything
             *        else (garbage / a future source kind) reads as UNKNOWN.
             */
            Source getSource() const
            {
                if (!isDataValid()) {
                    return Source::UNKNOWN;
                }
                switch (static_cast<uint8_t>(mData.f[SOURCE])) {
                    case static_cast<uint8_t>(Source::OPTICAL):  return Source::OPTICAL;
                    case static_cast<uint8_t>(Source::EXTERNAL): return Source::EXTERNAL;
                    default:                                     return Source::UNKNOWN;
                }
            }

            uint32_t getTimestamp() const   { return isDataValid() ? mData.getTimestamp() : 0; }
            uint64_t getTimestampUs() const { return isDataValid() ? mData.getTimestampUs() : 0; }

            /**
             * @brief Number of expected fields (7).
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class HeartRateEx */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_HEART_RATE_EX_HPP */
