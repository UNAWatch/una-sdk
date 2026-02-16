
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
    // constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;  // Commented out: HR values message type

    ///////////////////////////////////////
    //// Application custom structures
    ///////////////////////////////////////

    // Service --> GUI
    // struct HRValues : public SDK::MessageBase {  // Commented out: HR values message struct
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
        // bool updateHeartRate(float value, float trustLevel)  // Commented out: Send HR update to GUI
        // {
        //     if (auto req = SDK::make_msg<CustomMessage::HRValues>(mKernel)) {
        //         req->heartRate  = value;
        //         req->trustLevel = trustLevel;

        //         return req.send();
        //     }

        //     return false;
        // }

    private:
        const SDK::Kernel& mKernel;
    };

} // namespace CustomMessage

#pragma pack(pop)
