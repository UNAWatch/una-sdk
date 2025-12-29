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

#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

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

bool TouchGFXCommandProcessor::waitForFrameTick()
{
    mLastButtonCode = 0;

    // Called once
    if (mAppLifeCycleCallback && !mStartCallbackCalled) {
        mStartCallbackCalled = true;
        mAppLifeCycleCallback->onStart();
    }

    while (true) {
        SDK::MessageBase *msg = nullptr;
        bool messageQueued = false;

        // Wait for command (blocks until available)
        if(!mKernel.comm.getMessage(msg)) {
#if defined(SIMULATOR)
            return false;
#else
            continue;
#endif
        }

        switch (msg->getType()) {

            case SDK::MessageType::COMMAND_APP_STOP: {
                msg->setResult(SDK::MessageResult::SUCCESS);
                // We must release the message here because we are exiting this app.
                mKernel.comm.releaseMessage(msg);

                if (mAppLifeCycleCallback) {
                    // Cleanup recourses
                    mAppLifeCycleCallback->onStop();
                }
                // Waiting for the kernel to kill this app
                mKernel.sys.exit(0); // no return
                return true;
            } break;

            case SDK::MessageType::EVENT_GUI_TICK: {
                msg->setResult(SDK::MessageResult::SUCCESS);
                // We must release the message here because we are exiting this method.
                mKernel.comm.releaseMessage(msg);

                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onFrame();
                }
                return false; // Allow TouchGFX make frame
            } break;

            case SDK::MessageType::EVENT_BUTTON: {
                handleEvent(static_cast<SDK::Message::EventButton*>(msg));
                msg->setResult(SDK::MessageResult::SUCCESS);
            } break;

            case SDK::MessageType::COMMAND_APP_GUI_RESUME: {
                msg->setResult(SDK::MessageResult::SUCCESS);

                mIsGuiResumed = true;
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onResume();
                }
            } break;

            case SDK::MessageType::COMMAND_APP_GUI_SUSPEND: {
                msg->setResult(SDK::MessageResult::SUCCESS);
                mIsGuiResumed = false;
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onSuspend();
                }
            } break;


            default:
                if (SDK::isApplicationSpecificMessage(msg->getType()) && mCustomMessageHandler) {

                    // Remove oldest message if queue is full
                    if (mUserQueue.full()) {
                        LOG_WARNING("Queue for custom messages is full\n");
                        auto v = mUserQueue.pop();
                        if (v) {
                            auto msg = *v;
                            msg->setResult(SDK::MessageResult::FAIL);
                            mKernel.comm.sendResponse(msg);
                            mKernel.comm.releaseMessage(msg);
                        }
                    }
                    // Try to save message
                    messageQueued = mUserQueue.push(msg);

                } else {
                    msg->setResult(SDK::MessageResult::FAIL);
                    mKernel.comm.sendResponse(msg);
                }
                break;
        }

        if (messageQueued) {
            // The message must be process and release in callCustomMessageHandler()
            continue;
        }

        // Set the result if the message was not processed
        if (msg->getResult() == SDK::MessageResult::PENDING) {
            msg->setResult(SDK::MessageResult::FAIL);
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
        mKernel.comm.sendMessage(msg, 1000);
        mKernel.comm.releaseMessage(msg);
    }
}

void TouchGFXCommandProcessor::callCustomMessageHandler()
{
    while (!mUserQueue.empty()) {
        auto v = mUserQueue.pop();

        if (v) {
            auto msg = *v;
            bool result = false;
            if (mCustomMessageHandler) {
                result = mCustomMessageHandler->customMessageHandler(msg);
            }
            msg->setResult(result ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
            mKernel.comm.sendResponse(msg);
            mKernel.comm.releaseMessage(msg);
        }
    };
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
