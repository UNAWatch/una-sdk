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
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/UnaLogger/Logger.h"
#include "SDK/AppSystem/SrvBootstrap.hpp"


/**
 * @brief  Global kernel pointer defined in system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/*
 * @brief Main entry point for an application service.
 * @retval int
 */
int main()
{
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);
    SDK::KernelProviderService::CreateInstance(&kernel);
    Logger_init(kernel.logger);

    SDK::Service::Bootstrap bootstrap;
    bootstrap.run();

    return 0;
}

