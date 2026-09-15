/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Images and its link to the kernel.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    // The port owns the kernel's lifecycle slot and forwards each event here.
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator!\n");
    LOG_INFO("Keys: 1 = L1 (toggle scaled / plain image), 3 = R1 (jump, plain image), 4 = R2 (exit), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);

    // No return on the watch: the kernel stops the GUI process. In the
    // simulator this makes the port's frame loop return.
    mKernel.sys.exit();
}

void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    // The port has already invalidated the screen, so the kernel gets a full
    // frame; nothing to do for Images.
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
