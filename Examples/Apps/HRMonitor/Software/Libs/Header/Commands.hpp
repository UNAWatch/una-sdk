
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Application custom commands

    // Service --> GUI
    constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;

    // Service --> GUI
    struct HRValues : public SDK::MessageBase {
        float heartRate;
        float trustLevel;

        HRValues()
            : SDK::MessageBase(HR_VALUES)
            , heartRate(0.0f)
            , trustLevel(0.0f)
        {}

        explicit HRValues(float heartRate, float trustLevel)
            : HRValues()
        {
            this->heartRate  = heartRate;
            this->trustLevel = trustLevel;
        }
    };


} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
