
#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include <array>
#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    ///////////////////////////////////////
    //// Application custom commands
    ///////////////////////////////////////

    // Service --> GUI
    constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;
    constexpr SDK::MessageType::Type LOCATION_VALUES = 0x00000002;
    constexpr SDK::MessageType::Type ELEVATION_VALUES = 0x00000003;
    constexpr SDK::MessageType::Type ACCELEROMETER_VALUES = 0x00000004;
    constexpr SDK::MessageType::Type STEP_COUNTER_VALUES = 0x00000005;
    constexpr SDK::MessageType::Type FLOORS_VALUES = 0x00000006;
    constexpr SDK::MessageType::Type COMPASS_VALUES = 0x00000007;
    constexpr SDK::MessageType::Type STATS_VALUES = 0x00000008;
    constexpr SDK::MessageType::Type BATTERY_VALUES = 0x0000000A;
    constexpr SDK::MessageType::Type PRESSURE_VALUES = 0x0000000B;

    ///////////////////////////////////////
    //// Application custom structures
    ///////////////////////////////////////

    // Service --> GUI
    struct HRValues : public SDK::MessageBase {
        float heartRate;
        float trustLevel;

        HRValues()
            : SDK::MessageBase(HR_VALUES)
            , heartRate()
            , trustLevel()
        {}

        explicit HRValues(float heartRate, float trustLevel)
            : HRValues()
        {
            this->heartRate  = heartRate;
            this->trustLevel = trustLevel;
        }
    };

    // Service --> GUI
    struct LocationValues : public SDK::MessageBase {
        uint64_t timestamp;
        double latitude;
        double longitude;
        double altitude;

        LocationValues()
            : SDK::MessageBase(LOCATION_VALUES)
            , timestamp()
            , latitude()
            , longitude()
            , altitude()
        {}

        explicit LocationValues(uint64_t timestamp, double latitude, double longitude, double altitude)
            : LocationValues()
        {
            this->timestamp = timestamp;
            this->latitude  = latitude;
            this->longitude = longitude;
            this->altitude  = altitude;
        }
    };

    // Service --> GUI
    struct ElevationValues : public SDK::MessageBase {
        uint64_t timestamp;
        float elevation;

        ElevationValues()
            : SDK::MessageBase(ELEVATION_VALUES)
            , timestamp()
            , elevation()
        {}

        explicit ElevationValues(uint64_t timestamp, float elevation)
            : ElevationValues()
        {
            this->timestamp = timestamp;
            this->elevation = elevation;
        }
    };

    // Service --> GUI
    struct AccelerometerValues : public SDK::MessageBase {
        uint64_t timestamp;
        float x;
        float y;
        float z;

        AccelerometerValues()
            : SDK::MessageBase(ACCELEROMETER_VALUES)
            , timestamp()
            , x()
            , y()
            , z()
        {}

        explicit AccelerometerValues(uint64_t timestamp, float x, float y, float z)
            : AccelerometerValues()
        {
            this->timestamp = timestamp;
            this->x         = x;
            this->y         = y;
            this->z         = z;
        }
    };

    // Service --> GUI
    struct StepCounterValues : public SDK::MessageBase {
        uint64_t timestamp;
        uint32_t steps;

        StepCounterValues()
            : SDK::MessageBase(STEP_COUNTER_VALUES)
            , timestamp()
            , steps()
        {}

        explicit StepCounterValues(uint64_t timestamp, uint32_t steps)
            : StepCounterValues()
        {
            this->timestamp = timestamp;
            this->steps     = steps;
        }
    };

    // Service --> GUI
    struct FloorsValues : public SDK::MessageBase {
        uint64_t timestamp;
        uint32_t floors;

        FloorsValues()
            : SDK::MessageBase(FLOORS_VALUES)
            , timestamp()
            , floors()
        {}

        explicit FloorsValues(uint64_t timestamp, uint32_t floors)
            : FloorsValues()
        {
            this->timestamp = timestamp;
            this->floors    = floors;
        }
    };

    // Service --> GUI
    struct CompassValues : public SDK::MessageBase {
        uint64_t timestamp;
        float heading;

        CompassValues()
            : SDK::MessageBase(COMPASS_VALUES)
            , timestamp()
            , heading()
        {}

        explicit CompassValues(uint64_t timestamp, float heading)
            : CompassValues()
        {
            this->timestamp = timestamp;
            this->heading   = heading;
        }
    };

    struct StatsValues : public SDK::MessageBase {
        float serviceCpuPct;
        float guiCpuPct;
        float txMsgRate;
        float rxMsgRate;
        float txByteRate;
        float rxByteRate;

        StatsValues()
            : SDK::MessageBase(STATS_VALUES)
            , serviceCpuPct(0)
            , guiCpuPct(0)
            , txMsgRate(0)
            , rxMsgRate(0)
            , txByteRate(0)
            , rxByteRate(0)
        {}

        explicit StatsValues(float serviceCpuPct, float guiCpuPct, float txMsgRate,
                 float rxMsgRate, float txByteRate, float rxByteRate)
            : StatsValues()
        {
            this->serviceCpuPct = serviceCpuPct;
            this->guiCpuPct     = guiCpuPct;
            this->txMsgRate     = txMsgRate;
            this->rxMsgRate     = rxMsgRate;
            this->txByteRate    = txByteRate;
            this->rxByteRate    = rxByteRate;
        }
    };

    constexpr SDK::MessageType::Type RTC_VALUES = 0x00000009;

    struct RtcValues : public SDK::MessageBase {
        uint32_t time;  // ms since boot or unix sec

        RtcValues()
            : SDK::MessageBase(RTC_VALUES)
            , time(0)
        {}

        explicit RtcValues(uint32_t time)
            : RtcValues()
        {
            this->time = time;
        }
    };

    // Service --> GUI
    struct BatteryValues : public SDK::MessageBase {
        float level;

        BatteryValues()
            : SDK::MessageBase(BATTERY_VALUES)
            , level(0.0f)
        {}

        explicit BatteryValues(float level)
            : BatteryValues()
        {
            this->level = level;
        }
    };

    // Service --> GUI
    struct PressureValues : public SDK::MessageBase {
        uint64_t timestamp;
        float pressure;

        PressureValues()
            : SDK::MessageBase(PRESSURE_VALUES)
            , timestamp(0)
            , pressure(0.0f)
        {}

        explicit PressureValues(uint64_t timestamp, float pressure)
            : PressureValues()
        {
            this->timestamp = timestamp;
            this->pressure  = pressure;
        }
    };


} // namespace CustomMessage

#pragma pack(pop)
