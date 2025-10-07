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

#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/AppSystem/SvcBootstrap.hpp"
#include "SDK/AppSystem/UserAppEntry.hpp"
#include "Service.hpp"

#define LOG_MODULE_PRX      "main::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

////////////////////////////////////////////////////////////////////////////////
//// Logger's callbacks
////////////////////////////////////////////////////////////////////////////////

static uint32_t LoggerGetTicks()
{
    return SDK::KernelProviderService::GetInstance().getKernel().app.getTimeMs();
}

static void LoggerPrint(const char* str)
{
    SDK::KernelProviderService::GetInstance().getKernel().app.log(str);
}

extern "C" int _gettimeofday (struct timeval *__restrict __p,
              void *__restrict __tz)    // __tz deprecated parameter
{
    return 42;
}

////////////////////////////////////////////////////////////////////////////////
//// Main
////////////////////////////////////////////////////////////////////////////////

int main()
{
    ///////////////////////////
    //// Build the kernel
    ///////////////////////////

    SDK::Kernel kernel = SDK::KernelBuilder::make();
    SDK::KernelProviderService::CreateInstance(&kernel);

    ///////////////////////////
    //// Init logger
    ///////////////////////////

    Logger_init(LoggerPrint);
    Logger_setTimeFunc(LoggerGetTicks);

    ///////////////////////////
    //// Start
    ///////////////////////////

    SDK::Service::Bootstrap bootstrap;
    bootstrap.run();

    return 0;
}

