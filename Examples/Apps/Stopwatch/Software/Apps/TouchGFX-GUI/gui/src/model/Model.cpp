#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    , mSender(mKernel)
    , mState{}
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

    setCapabilities();

#if defined(SIMULATOR)
    std::string fsPath = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Simulator.\n");
    LOG_INFO("FS path: [%s].\n", fsPath.c_str());
    LOG_INFO("Buttons: 1=L1 2=L2 3=R1 4=R2\n\n");
#endif
}


// Controls

FrontendApplication& Model::application()
{
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

void Model::tick()
{
    if (mInvalidate) {
        mInvalidate = false;
        application().invalidate();
    }
}

void Model::setCapabilities()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestSetCapabilities>();
    if (msg) {
        msg->enUsbChargingScreen = true;
        msg->enPhoneNotification = true;
        msg->enMusicControl = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");

    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);

    mKernel.sys.exit();
    // On simulator sys.exit() only sets a flag -- the current tick completes normally.
}


// IGuiLifeCycleCallback

void Model::onStart()
{
    LOG_INFO("Started\n");

    // The service publishes a snapshot when it is told the GUI is up, so this
    // is only a safety net for the case where that message beat our handler
    // being registered.
    mSender.requestState();
}

void Model::onResume()
{
    mInvalidate = true;

    // The stopwatch may have moved on while the GUI was suspended.
    mSender.requestState();
}

void Model::onSuspend()
{
    // The kernel may park the GUI at any time. The service is untouched and
    // keeps timing, so there is nothing to save here.
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}


// ICustomMessageHandler
bool Model::customMessageHandler(SDK::MessageBase *msg)
{
    if (msg->getType() != CustomMessage::STOPWATCH_STATE) {
        return false;
    }

    mState = static_cast<CustomMessage::StopwatchState *>(msg)->state;

    if (modelListener) {
        modelListener->onStopwatchChanged(mState);
    }
    return true;
}
