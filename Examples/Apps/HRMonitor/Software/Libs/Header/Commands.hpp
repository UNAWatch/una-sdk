
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Kernel/Kernel.hpp"

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
    };


// Helper wrapper
class Sender {
public:
    Sender(const SDK::Kernel &kernel) :
            mKernel(kernel)
    {
    }
    virtual ~Sender() = default;

    // Service --> GUI
    bool heartRate(float value, float trustLevel)
    {
        bool status = false;
        auto *msg = mKernel.comm.allocateMessage<CustomMessage::HRValues>();
        if (msg) {
            msg->heartRate  = value;
            msg->trustLevel = trustLevel;
            status = mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        return status;
    }

private:
    const SDK::Kernel &mKernel;
};


} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
