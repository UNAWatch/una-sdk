/**
 ******************************************************************************
 * @file    TouchGFXCommandProcessor.cpp
 * @date    11-12-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Central TouchGFX command processor.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "TouchGFXCommandProcessor"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageTypes.hpp"


namespace SDK
{

TouchGFXCommandProcessor::TouchGFXCommandProcessor()
          : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
          , mStartCallbackCalled(false)
          , mIsGuiResumed(false)
          , mLastButtonCode(0)
          , mAppLifeCycleCallback(nullptr)
          , mCustomMessageHandler(nullptr)
{
}

TouchGFXCommandProcessor::~TouchGFXCommandProcessor()
{
}

void TouchGFXCommandProcessor::waitForFrameTick()
{
    mLastButtonCode = 0;

    // Called once
    if (mAppLifeCycleCallback && !mStartCallbackCalled) {
        mStartCallbackCalled = true;
        mAppLifeCycleCallback->onStart();
    }

    SDK::MessageBase *msg = nullptr;

    while (true) {

        // Wait for command (blocks until available)
        if(!mKernel.comm.getMessage(msg)) {
            continue;
        }

        switch (msg->getType()) {

            case SDK::MessageType::COMMAND_APP_STOP: {
                mKernel.comm.releaseMessage(msg);
                if (mAppLifeCycleCallback) {
                    // Cleanup recourses
                    mAppLifeCycleCallback->onStop();
                }
                // Waiting for the kernel to kill this app
                mKernel.sys.exit(0); // no return
            } break;

            case SDK::MessageType::EVENT_GUI_TICK: {
                mKernel.comm.releaseMessage(msg);
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onFrame();
                }
                return; // Allow TouchGFX make frame
            } break;

            case SDK::MessageType::EVENT_BUTTON: {
                handleEvent(static_cast<SDK::Message::EventButton*>(msg));
            }

            case SDK::MessageType::COMMAND_APP_GUI_RESUME: {
                mIsGuiResumed = true;
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onResume();
                }
            } break;

            case SDK::MessageType::COMMAND_APP_GUI_SUSPEND: {
                mIsGuiResumed = false;
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onSuspend();
                }
            } break;


            default:
                if (SDK::isApplicationSpecificMessage(msg->getType())) {
                    if (mCustomMessageHandler) {
                        mCustomMessageHandler->customMessageHandler(msg);
                    }
                }
                break;
        }

        // Release message after processing
        mKernel.comm.releaseMessage(msg);

    }

}

bool TouchGFXCommandProcessor::getKeySample(uint8_t &key)
{
    key = mLastButtonCode;
    return mLastButtonCode != '\0';
}

void TouchGFXCommandProcessor::writeDisplayFrameBuffer(const uint8_t* data)
{
    if (!data || !mIsGuiResumed) {
        return;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestDisplayUpdate>();
    if (msg) {
        msg->pBuffer = data;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void TouchGFXCommandProcessor::handleEvent(SDK::Message::EventButton* msg)
{
    if (!mIsGuiResumed) {
        mLastButtonCode = '\0';
        return;
    }

    if (msg->event == SDK::Message::EventButton::Event::CLICK) {
        switch (msg->id) {
            case SDK::Message::EventButton::Id::SW1: mLastButtonCode = '1'; break;
            case SDK::Message::EventButton::Id::SW2: mLastButtonCode = '3'; break;
            case SDK::Message::EventButton::Id::SW3: mLastButtonCode = '2'; break;
            case SDK::Message::EventButton::Id::SW4: mLastButtonCode = '4'; break;
            default: break;
        }
    }
}

} // namespace SDK
