#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#include "Commands.hpp"

#include <ctime>

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

    mTime = now();

#if defined(SIMULATOR)
    LOG_INFO("Simulator.\n");
#endif
}

WallTime Model::now() const
{
    std::tm local {};
    std::time_t utc = std::time(nullptr);

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local, &utc);
#else
    localtime_r(&utc, &local);
#endif

    return { static_cast<uint8_t>(local.tm_hour),
             static_cast<uint8_t>(local.tm_min),
             static_cast<uint8_t>(local.tm_mday),
             static_cast<uint8_t>(local.tm_wday) };
}


// Controls

FrontendApplication& Model::application()
{
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

void Model::tick()
{
    if (!mResumed) {
        return;
    }

    mResumed = false;

    // Taking the reading here rather than in onResume() keeps every touch of a
    // widget on the thread that draws. onResume() is dispatched from
    // waitForFrameTick(), which the simulator drives on a thread of its own.
    adopt(now());

    // Whatever the kernel drew over the clockface is still on the display, so
    // the whole screen has to be painted again, not just the hands.
    application().invalidate();
}

void Model::adopt(const WallTime &time)
{
    if (mTime == time) {
        return;
    }

    mTime = time;

    if (modelListener) {
        modelListener->onTime(mTime);
    }
}


// IGuiLifeCycleCallback

void Model::onStart()
{
    LOG_INFO("Started\n");
}

void Model::onResume()
{
    // Only a flag: the reading and the repaint both happen in tick(), on the
    // thread that draws. The face may have been off screen across a minute
    // boundary, and the next push is not due until the one after it.
    mResumed = true;
}

void Model::onSuspend()
{
    // Nothing is running that a suspend could interrupt.
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}


// ICustomMessageHandler

bool Model::customMessageHandler(SDK::MessageBase *message)
{
    if (!message) {
        return false;
    }

    switch (message->getType()) {
        case CustomMessage::TIME: {
            auto *msg = static_cast<CustomMessage::Time*>(message);
            adopt(WallTime{ msg->hour, msg->minute, msg->mday, msg->wday });
        } break;

        case CustomMessage::BATTERY: {
            auto *msg = static_cast<CustomMessage::Battery*>(message);
            if (mBatteryLevel != msg->level) {
                mBatteryLevel = msg->level;
                if (modelListener) {
                    modelListener->onBatteryLevel(mBatteryLevel);
                }
            }
        } break;

        case CustomMessage::ALERTS_MUTED: {
            auto *msg = static_cast<CustomMessage::AlertsMuted*>(message);
            if (mAlertsMuted != msg->muted) {
                mAlertsMuted = msg->muted;
                if (modelListener) {
                    modelListener->onAlertsMuted(mAlertsMuted);
                }
            }
        } break;

        default:
            break;
    }

    return true;
}
