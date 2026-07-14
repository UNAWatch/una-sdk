/**
 ******************************************************************************
 * @file    main.сpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   The application entry point
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/UnaLogger/Logger.h"

/**
 * @brief Global kernel pointer provided by system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

extern "C" void touchgfx_init(void);
extern "C" void touchgfx_components_init(void);
extern "C" void touchgfx_taskEntry(void);


/*
 * @brief Main entry point for a GUI application based on the TouchGFX framework.
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

    touchgfx_components_init();
    touchgfx_init();
    touchgfx_taskEntry();   // No return

    return 0;
}

