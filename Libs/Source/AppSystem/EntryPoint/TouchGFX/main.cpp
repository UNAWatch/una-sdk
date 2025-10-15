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

#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/UnaLogger/Logger.h"

/**
 * @brief  Global kernel pointer defined in system.cpp.
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
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);
    SDK::KernelProviderGUI::CreateInstance(&kernel);
    Logger_init(kernel.logger);

    touchgfx_components_init();
    touchgfx_init();
    touchgfx_taskEntry();   // No return

    return 0;
}

