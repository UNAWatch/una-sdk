/**
 ******************************************************************************
 * @file    TouchGFXCommandProcessor.hpp
 * @date    11-12-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Central TouchGFX command processor.
 ******************************************************************************
 *
 ******************************************************************************
 */



//TODO: Move this functionality to the Kernel class ?


#pragma once

#include <cstdint>
#include <cstddef>
#include <queue>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Tools/FixedQueue.hpp"

namespace SDK
{

/**
 * @brief Central TouchGFX command processor.
 *
 * Singleton class that manages TouchGFX lifecycle and kernel commands.
 */
class TouchGFXCommandProcessor
{

public:

    static TouchGFXCommandProcessor& GetInstance()
    {
        static TouchGFXCommandProcessor sInstance;
        return sInstance;
    }

    void setAppLifeCycleCallback(SDK::Interface::IGuiLifeCycleCallback* cb) { mAppLifeCycleCallback = cb; }

    void setCustomMessageHandler(SDK::Interface::ICustomMessageHandler* h) { mCustomMessageHandler = h; }


    void waitForFrameTick();

    bool getKeySample(uint8_t &key);

    void writeDisplayFrameBuffer(const uint8_t* data);

    // Called before Model::tick()
    void callCustomMessageHandler();

private:

    TouchGFXCommandProcessor();
    virtual ~TouchGFXCommandProcessor();

    // Prevent copying
    TouchGFXCommandProcessor(const TouchGFXCommandProcessor&) = delete;
    TouchGFXCommandProcessor& operator=(const TouchGFXCommandProcessor&) = delete;

    const SDK::Kernel &mKernel;
    bool mStartCallbackCalled;
    bool mIsGuiResumed;
    uint8_t mLastButtonCode;
    SDK::Interface::IGuiLifeCycleCallback *mAppLifeCycleCallback;
    SDK::Interface::ICustomMessageHandler *mCustomMessageHandler;
    SDK::Tools::FixedQueue<SDK::MessageBase*, 10> mUserQueue {};

    void handleEvent(SDK::Message::EventButton* msg);

};

} // namespace SDK

