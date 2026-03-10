
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

    private:
        const SDK::Kernel& mKernel;
    };

} // namespace CustomMessage

#pragma pack(pop)
