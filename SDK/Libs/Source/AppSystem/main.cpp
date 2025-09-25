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

#include "SDK/AppSystem/AppKernel.hpp"
#include "SDK/AppSystem/UserAppEntry.hpp"

#define LOG_MODULE_PRX      "main::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

////////////////////////////////////////////////////////////////////////////////
//// Logger's callbacks
////////////////////////////////////////////////////////////////////////////////

static uint32_t LoggerGetTicks()
{
    return SDK::Kernel::GetInstance().app.getTimeMs();
}

static void LoggerPrint(const char* str)
{
    SDK::Kernel::GetInstance().app.log(str);
}

////////////////////////////////////////////////////////////////////////////////
//// Main
////////////////////////////////////////////////////////////////////////////////

int main()
{
    Logger_init(LoggerPrint);
    Logger_setTimeFunc(LoggerGetTicks);

    UserAppEntry();

    return 0;
}

