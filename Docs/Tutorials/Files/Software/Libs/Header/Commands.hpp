
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
    //// Application custom types
    ///////////////////////////////////////

    enum ActivityType {
        RUNNING = 0,
        CYCLING = 1,
        SWIMMING = 2,
        WALKING = 3,
        _MAX_ACTIVITY_TYPE
    };
    
    enum DisplayMode {
        SIMPLE = 0,
        DETAILED = 1,
        COMPACT = 2,
        _MAX_DISPLAY_MODE
    };

    ///////////////////////////////////////
    //// Application custom commands
    ///////////////////////////////////////

    // Service --> GUI
    /*
     * HR_VALUES Message Type:
     * - Unique identifier for heart rate update messages
     * - Must be unique across all custom message types in the app
     * - Used to route messages to correct handler in GUI
     */
    // constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;

    // Settings messages
    constexpr SDK::MessageType::Type GET_SETTINGS = 0x00000002;
    constexpr SDK::MessageType::Type SET_SETTINGS = 0x00000003;
    constexpr SDK::MessageType::Type SETTINGS_VALUES = 0x00000004;

    ///////////////////////////////////////
    //// Application custom structures
    ///////////////////////////////////////

    // Service --> GUI
    /*
     * HRValues Message Struct:
     * - Inherits from SDK::MessageBase for SDK messaging
     * - The default constructor sets the message type to HR_VALUES
     * - A second constructor fills the fields and delegates to the first, which
     *   is what lets a caller send it in one line with SDK::send_msg
     * - Contains HR data: heartRate (BPM) and trustLevel (0.0-1.0)
     */
    // struct HRValues : public SDK::MessageBase {
    //     float heartRate;
    //     float trustLevel;

    //     HRValues()
    //         : SDK::MessageBase(HR_VALUES)
    //         , heartRate()
    //         , trustLevel()
    //     {}

    //     explicit HRValues(float heartRate, float trustLevel)
    //         : HRValues()
    //     {
    //         this->heartRate  = heartRate;
    //         this->trustLevel = trustLevel;
    //     }
    // };

    // GUI --> Service
    struct GetSettings : public SDK::MessageBase {
        GetSettings()
            : SDK::MessageBase(GET_SETTINGS)
        {}
    };

    // GUI --> Service
    struct SetSettings : public SDK::MessageBase {
        int32_t decimalCounter;
        int activityType;
        int displayMode;

        SetSettings()
            : SDK::MessageBase(SET_SETTINGS)
            , decimalCounter(0)
            , activityType(0)
            , displayMode(0)
        {}

        explicit SetSettings(int32_t decimalCounter, int activityType, int displayMode)
            : SetSettings()
        {
            this->decimalCounter = decimalCounter;
            this->activityType   = activityType;
            this->displayMode    = displayMode;
        }
    };

    // Service --> GUI
    struct SettingsValues : public SDK::MessageBase {
        int32_t decimalCounter;
        int activityType;
        int displayMode;

        SettingsValues()
            : SDK::MessageBase(SETTINGS_VALUES)
            , decimalCounter(0)
            , activityType(0)
            , displayMode(0)
        {}

        explicit SettingsValues(int32_t decimalCounter, int activityType, int displayMode)
            : SettingsValues()
        {
            this->decimalCounter = decimalCounter;
            this->activityType   = activityType;
            this->displayMode    = displayMode;
        }
    };

} // namespace CustomMessage

#pragma pack(pop)
