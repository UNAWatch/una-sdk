
#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include <array>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

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

    ///////////////////////////////////////
    //// Application custom structures
    ///////////////////////////////////////

    // Service --> GUI
    /*
     * HRValues Message Struct:
     * - Inherits from SDK::MessageBase for SDK messaging
     * - Constructor initializes message type to HR_VALUES
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
    // };


    ///////////////////////////////////////
    //// Wrappers
    ///////////////////////////////////////

    class GUISender {
    public:
        GUISender(const SDK::Kernel& kernel) : mKernel(kernel)
        {}

        virtual ~GUISender() = default;

        // Service --> GUI
        /*
         * updateHeartRate Method:
         * - Creates and sends HR update message to GUI
         * - Uses SDK::make_msg to allocate message from kernel pool
         * - Returns true if message sent successfully
         * - Automatically cleaned up by SDK after delivery
         */
        // bool updateHeartRate(float value, float trustLevel)
        // {
        //     if (auto req = SDK::make_msg<CustomMessage::HRValues>(mKernel)) {
        //         req->heartRate  = value;
        //         req->trustLevel = trustLevel;

        //         return req.send();  // Send to GUI via SDK messaging
        //     }

        //     return false;
        // }

    private:
        const SDK::Kernel& mKernel;
    };

} // namespace CustomMessage

#pragma pack(pop)
