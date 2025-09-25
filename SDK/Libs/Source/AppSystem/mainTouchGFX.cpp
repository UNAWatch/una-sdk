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

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"

#define LOG_MODULE_PRX      "main::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

extern "C" void touchgfx_init(void);
extern "C" void touchgfx_components_init(void);
extern "C" void touchgfx_taskEntry(void);

extern const IKernel* kernel;

////////////////////////////////////////////////////////////////////////////////
//// Logger's callbacks
////////////////////////////////////////////////////////////////////////////////

static uint32_t LoggerGetTicks()
{
    return kernel->app.getTimeMs();
}

static void LoggerPrint(const char* str)
{
    kernel->app.log(str);
}

////////////////////////////////////////////////////////////////////////////////
//// Main
////////////////////////////////////////////////////////////////////////////////

int main()
{
    SDK::Kernel kernel = SDK::KernelBuilder::make();
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    Logger_init(LoggerPrint);
    Logger_setTimeFunc(LoggerGetTicks);

    touchgfx_components_init();
    touchgfx_init();
    touchgfx_taskEntry();   // No return

    return 0;
}

