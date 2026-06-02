/**
 ******************************************************************************
 * @file    main.cpp
 * @brief   The GUI application entry point (LVGL).
 *
 * Mirrors EntryPoint/TouchGFX/main.cpp exactly except the GUI bring-up tail:
 * MX_LVGL_Init() then lvgl_taskEntry() (Design.md §1.6).
 ******************************************************************************
 */

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Port/LVGL/app_lvgl.h"          // MX_LVGL_Init
#include "SDK/Port/LVGL/lv_port_lifecycle.h" // lvgl_taskEntry

/**
 * @brief Global kernel pointer provided by system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/*
 * @brief Main entry point for a GUI application based on the LVGL framework.
 * @retval int
 */
int main()
{
    // Create Kernel instance
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);

    // Initialize KernelProvider to allow global kernel access
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    // Initialize application logger
    Logger_init(kernel.log);

    MX_LVGL_Init();   // lv_init + display/indev/lifecycle/theme
    lvgl_taskEntry(); // no return

    return 0;
}
