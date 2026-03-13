
#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Kernel/Kernel.hpp"

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
    };

    constexpr SDK::MessageType::Type RTC_VALUES = 0x00000009;

    struct RtcValues : public SDK::MessageBase {
        uint32_t time;  // ms since boot or unix sec

        RtcValues()
            : SDK::MessageBase(RTC_VALUES)
            , time(0)
        {}
    };

    // Service --> GUI
    struct BatteryValues : public SDK::MessageBase {
        float level;

        BatteryValues()
            : SDK::MessageBase(BATTERY_VALUES)
            , level(0.0f)
        {}
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
    };


    ///////////////////////////////////////
    //// Wrappers
    ///////////////////////////////////////

    class GUISender {
    public:
        GUISender(const SDK::Kernel& kernel) : mKernel(kernel)
        {}

        virtual ~GUISender() = default;

        // Service --> GUI
        bool updateHeartRate(float value, float trustLevel)
        {
            if (auto req = SDK::make_msg<CustomMessage::HRValues>(mKernel)) {
                req->heartRate  = value;
                req->trustLevel = trustLevel;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
        bool updateLocation(uint64_t timestamp, double latitude, double longitude, double altitude)
        {
            if (auto req = SDK::make_msg<CustomMessage::LocationValues>(mKernel)) {
                req->timestamp = timestamp;
                req->latitude  = latitude;
                req->longitude = longitude;
                req->altitude  = altitude;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
        bool updateElevation(uint64_t timestamp, float elevation)
        {
            if (auto req = SDK::make_msg<CustomMessage::ElevationValues>(mKernel)) {
                req->timestamp = timestamp;
                req->elevation = elevation;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
        bool updateAccelerometer(uint64_t timestamp, float x, float y, float z)
        {
            if (auto req = SDK::make_msg<CustomMessage::AccelerometerValues>(mKernel)) {
                req->timestamp = timestamp;
                req->x = x;
                req->y = y;
                req->z = z;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
        bool updateStepCounter(uint64_t timestamp, uint32_t steps)
        {
            if (auto req = SDK::make_msg<CustomMessage::StepCounterValues>(mKernel)) {
                req->timestamp = timestamp;
                req->steps = steps;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
        bool updateFloors(uint64_t timestamp, uint32_t floors)
        {
            if (auto req = SDK::make_msg<CustomMessage::FloorsValues>(mKernel)) {
                req->timestamp = timestamp;
                req->floors = floors;

                return req.send();
            }

            return false;
        }

        // Service --> GUI
    bool updateCompass(uint64_t timestamp, float heading)
    {
        if (auto req = SDK::make_msg<CustomMessage::CompassValues>(mKernel)) {
            req->timestamp = timestamp;
            req->heading = heading;

            return req.send();
        }

        return false;
    }

    bool updateStats(float serviceCpuPct, float guiCpuPct, float txMsgRate, float rxMsgRate, float txByteRate, float rxByteRate)
    {
        if (auto req = SDK::make_msg<CustomMessage::StatsValues>(mKernel)) {
            req->serviceCpuPct = serviceCpuPct;
            req->guiCpuPct = guiCpuPct;
            req->txMsgRate = txMsgRate;
            req->rxMsgRate = rxMsgRate;
            req->txByteRate = txByteRate;
            req->rxByteRate = rxByteRate;

            return req.send();
        }

        return false;
    }

    bool updateRtc(uint32_t time)
    {
        if (auto req = SDK::make_msg<CustomMessage::RtcValues>(mKernel)) {
            req->time = time;

            return req.send();
        }

        return false;
    }

    // Service --> GUI
    bool updateBattery(float level)
    {
        if (auto req = SDK::make_msg<CustomMessage::BatteryValues>(mKernel)) {
            req->level = level;

            return req.send();
        }

        return false;
    }

    // Service --> GUI
    bool updatePressure(uint64_t timestamp, float pressure)
    {
        if (auto req = SDK::make_msg<CustomMessage::PressureValues>(mKernel)) {
            req->timestamp = timestamp;
            req->pressure = pressure;

            return req.send();
        }

        return false;
    }

private:
    const SDK::Kernel& mKernel;
    };

} // namespace CustomMessage

#pragma pack(pop)
