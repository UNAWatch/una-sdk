/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Waypoint and its link to the service.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    // The port owns the kernel's callback slots and forwards to these.
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator!\n");
    LOG_INFO("Keys: 3 = R1 (save the current position as the target), 4 = R2 (exit), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::saveTargetHere()
{
    LOG_INFO("asking the service to save the current position\n");
    SDK::send_msg<CustomMessage::SaveTargetHere>(mKernel);
}

void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    LOG_INFO("called\n");
}

void Model::onSuspend()
{
    LOG_INFO("called\n");
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

// Events from the service
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::NAV_UPDATE: {
            auto* m = static_cast<CustomMessage::NavUpdate*>(msg);
            mNav    = m->nav;
            if (modelListener) {
                modelListener->onNavUpdate(mNav);
            }
        } break;

        case CustomMessage::TARGET_SAVED: {
            auto* m = static_cast<CustomMessage::TargetSaved*>(msg);
            LOG_INFO("save outcome: %d\n", static_cast<int>(m->outcome));
            if (modelListener) {
                modelListener->onTargetSaved(m->outcome, m->targetLatitude, m->targetLongitude);
            }
        } break;

        default:
            break;
    }
    return true;
}
