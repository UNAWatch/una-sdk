#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#if defined(SIMULATOR)
    #include "touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp"
    #ifdef _WIN32
    #include "Windows.h"
    #endif
    #include <chrono>
    #include <ctime>
#endif

Model::Model()
    : modelListener(0)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator! \n");

    std::string fileStoreDir = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Path to files created by app:\n"
        "       [%s]\n", fileStoreDir.c_str());

    LOG_INFO("\n"
        "---------------------------------------------------\n"
        "|   For Simulation Button use keybaord Keys.      |\n"
        "|       Keys Keybaord:                            |\n"
        "|       1   L1,                                   |\n"
        "|       2   L2,                                   |\n"
        "|       3   R1,                                   |\n"
        "|       4   R2                                    |\n"
        "|                  /---------\\                    |\n"
        "|                 /           \\                   |\n"
        "| BUTTON UP   L1 |             | R1 BUTTON SELECT |\n"
        "|                |     UNA     |                  |\n"
        "|                |    WATCH    |                  |\n"
        "| BUTTON DOWN L2 |             | R2 BUTTON BACK   |\n"
        "|                 \\           /                   |\n"
        "|                  \\---------/                    |\n"
        "---------------------------------------------------\n"
    );
#endif
}

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

void Model::saveTargetHere()
{
    LOG_INFO("asking the service to save the current position\n");
    SDK::send_msg<CustomMessage::SaveTargetHere>(mKernel);
}

bool Model::customMessageHandler(SDK::MessageBase *msg)
{
    switch (msg->getType()) {
    case CustomMessage::NAV_UPDATE: {
        auto *m = static_cast<CustomMessage::NavUpdate *>(msg);
        mNav = m->nav;
        if (modelListener) {
            modelListener->onNavUpdate(mNav);
        }
    } break;

    case CustomMessage::TARGET_SAVED: {
        auto *m = static_cast<CustomMessage::TargetSaved *>(msg);
        LOG_INFO("save outcome: %d\n", static_cast<int>(m->outcome));
        if (modelListener) {
            modelListener->onTargetSaved(m->outcome, m->targetLatitude,
                                         m->targetLongitude);
        }
    } break;

    default:
        break;
    }

    return true;
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    // Cleanup recourses

    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);

    mKernel.sys.exit(); // No return for real app

    // !!! For TouchGFX Simulator !!!
    // This function only sets a flag.
    // The current TouchGFX loop will be completed, meaning that depending
    // on where this function was called, Model::tick(), Model::handleKeyEvent(),
    // as well as handleTickEvent() and handleKeyEvent() for the
    // current screen will be called.
}

// IUserApp implementation
void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    LOG_INFO("called\n");

    // Redraw screen
    mInvalidate = true;
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

void Model::onSuspend()
{
    LOG_INFO("called\n");
}
