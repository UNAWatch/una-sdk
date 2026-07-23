#pragma once

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/MessageBase.hpp"

namespace SDK
{

template<typename T>
class MessageGuard
{
public:
    MessageGuard(const SDK::Kernel& kernel, T* ptr) noexcept
        : mKernel(kernel)
        , mPtr(ptr)
    {}

    MessageGuard(const MessageGuard&)            = delete;
    MessageGuard& operator=(const MessageGuard&) = delete;

    MessageGuard(MessageGuard&&)            = delete;
    MessageGuard& operator=(MessageGuard&&) = delete;

    ~MessageGuard() noexcept
    {
        cleanup();
    }

    T*       get()           const noexcept { return mPtr;  }
    T*       operator->()    const noexcept { return mPtr;  }
    T&       operator*()     const noexcept { return *mPtr; }
    explicit operator bool() const noexcept { return mPtr != nullptr; }

    T* release() noexcept
    {
        T* tmp = mPtr;
        mPtr   = nullptr;
        return tmp;
    }

    bool send(uint32_t timeout = 0)
    {
        if (!mPtr) {
            return false;
        }

        return mKernel.comm.sendMessage(mPtr, timeout);
    }

    bool ok() const noexcept
    {
        if (!mPtr) {
            return false;
        }

        return mPtr->getResult() == SDK::MessageResult::SUCCESS;
    }

private:
    void cleanup() noexcept
    {
        if (mPtr) {
            mKernel.comm.releaseMessage(mPtr);
        }
        mPtr = nullptr;
    }

    const SDK::Kernel& mKernel;
    T*                 mPtr;
};

template<typename T, typename... Args>
MessageGuard<T> make_msg(const SDK::Kernel& kernel, Args&&... args)
{
    // static_cast<Args&&> is std::forward without pulling in <utility>.
    T* raw = kernel.comm.template allocateMessage<T>(static_cast<Args&&>(args)...);
    return MessageGuard<T>(kernel, raw);
}

inline SDK::MessageGuard<MessageID> make_msg(const SDK::Kernel&     kernel,
                                             SDK::MessageType::Type type)
{
    auto* raw = kernel.comm.template allocateMessage<MessageID>(); // 0 args
    raw->setType(type);
    return SDK::MessageGuard<MessageID>(kernel, raw);
}

// One-shot: allocate a T (forwarding any ctor args), send it, and release it on return -- the whole
// send path for a fire-and-forget message in one call, so the message type stays pure data and never
// needs to know about the kernel.
template<typename T, typename... Args>
bool send_msg(const SDK::Kernel& kernel, Args&&... args)
{
    auto guard = make_msg<T>(kernel, static_cast<Args&&>(args)...);
    if (!guard) {
        return false;
    }
    return guard.send();
}

} // namespace SDK
