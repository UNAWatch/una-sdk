/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Files and its link to the service.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
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
    LOG_INFO("Keys: 1 = L1 (value up), 2 = L2 (value down), 3 = R1 (save, next setting), "
             "4 = R2 (save, exit), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::requestSettings()
{
    SDK::send_msg<CustomMessage::GetSettings>(mKernel);
}

void Model::updateSettings(float decimalCounter, CustomMessage::ActivityType activityType,
                           CustomMessage::DisplayMode displayMode)
{
    // The file keeps the counter in tenths, as an integer.
    SDK::send_msg<CustomMessage::SetSettings>(mKernel, static_cast<int>(decimalCounter * 10),
                                             static_cast<int>(activityType), static_cast<int>(displayMode));
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
        case CustomMessage::SETTINGS_VALUES: {
            auto* m = static_cast<CustomMessage::SettingsValues*>(msg);
            LOG_DEBUG("decimalCounter %.1f, activityType %d, displayMode %d\n", m->decimalCounter / 10.0f,
                      m->activityType, m->displayMode);
            if (modelListener) {
                modelListener->onSettingsUpdate(m->decimalCounter / 10.0f,
                                                static_cast<CustomMessage::ActivityType>(m->activityType),
                                                static_cast<CustomMessage::DisplayMode>(m->displayMode));
            }
        } break;

        default:
            break;
    }
    return true;
}
