#pragma once

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/MessageBase.hpp"

namespace SDK
{

template<typename T>
class MessageGuard
{
public:
    MessageGuard(SDK::Kernel& kernel, T* ptr) noexcept
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

    SDK::Kernel& mKernel;
    T*           mPtr;
};

template<typename T>
MessageGuard<T> make_msg(SDK::Kernel& kernel)
{
    T* raw = kernel.comm.template allocateMessage<T>();
    return MessageGuard<T>(kernel, raw);
}

} // namespace SDK
