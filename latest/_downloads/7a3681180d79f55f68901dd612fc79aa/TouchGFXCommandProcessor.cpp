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
#include "SDK/GUI/Button.hpp"


namespace SDK
{

TouchGFXCommandProcessor::TouchGFXCommandProcessor()
          : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
          , mStartCallbackCalled(false)
          , mIsGuiResumed(false)
          , mAppLifeCycleCallback(nullptr)
          , mCustomMessageHandler(nullptr)
{
}

TouchGFXCommandProcessor::~TouchGFXCommandProcessor()
{
}

bool TouchGFXCommandProcessor::waitForFrameTick()
{
    // Called once
    if (mAppLifeCycleCallback && !mStartCallbackCalled) {
        mStartCallbackCalled = true;
        mAppLifeCycleCallback->onStart();
    }

    while (true) {
        SDK::MessageBase *msg = nullptr;
        bool messageQueued = false;


#if defined(SIMULATOR)
        if(!mKernel.comm.getMessage(msg,50)) {
            return false;
#else
         // Wait for command (blocks until available)
        if(!mKernel.comm.getMessage(msg)) {
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
                // Drop any pending codes so a half-finished press does not leak
                // into the next screen when the GUI resumes.
                mButtonCodes = {};
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
    // TouchGFX samples one key per frame; drain the queue one code at a time.
    auto code = mButtonCodes.pop();
    if (!code) {
        return false;
    }
    key = *code;
    return true;
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

namespace
{

// Click/press/release codes for one logical button, in kernel-id order.
struct ButtonCodes {
    uint8_t click;
    uint8_t press;
    uint8_t release;
};

// Map a kernel button id to its GUI codes. The id->position mapping is the same
// one the click path has always used: SW1->L1, SW2->R1, SW3->L2, SW4->R2.
bool getButtonCodes(SDK::Message::EventButton::Id id, ButtonCodes &out)
{
    namespace Btn = SDK::GUI::Button;
    using Id = SDK::Message::EventButton::Id;

    switch (id) {
        case Id::SW1: out = { Btn::L1, Btn::L1_PRESS, Btn::L1_RELEASE }; return true;
        case Id::SW2: out = { Btn::R1, Btn::R1_PRESS, Btn::R1_RELEASE }; return true;
        case Id::SW3: out = { Btn::L2, Btn::L2_PRESS, Btn::L2_RELEASE }; return true;
        case Id::SW4: out = { Btn::R2, Btn::R2_PRESS, Btn::R2_RELEASE }; return true;
        default: return false;
    }
}

} // namespace

void TouchGFXCommandProcessor::handleEvent(SDK::Message::EventButton* msg)
{
    if (!mIsGuiResumed) {
        return;
    }

    using Event = SDK::Message::EventButton::Event;

    ButtonCodes codes;
    if (!getButtonCodes(msg->id, codes)) {
        return;
    }

    uint8_t code = 0;
    switch (msg->event) {
        case Event::CLICK:   code = codes.click;   break;
        case Event::PRESS:   code = codes.press;   break;
        case Event::RELEASE: code = codes.release; break;
        // LONG_PRESS / HOLD_* are intentionally not forwarded: a screen derives
        // long press from the press/release pair using its own timing.
        default: return;
    }

    if (!mButtonCodes.push(code)) {
        // Queue full: drop the oldest so the most recent input stays responsive.
        mButtonCodes.pop();
        mButtonCodes.push(code);
    }
}

} // namespace SDK
