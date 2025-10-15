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
#include "SDK/AppSystem/UserAppEntry.hpp"


extern const IKernel* kernel;


////////////////////////////////////////////////////////////////////////////////
//// Main
////////////////////////////////////////////////////////////////////////////////

int main()
{
    SDK::Kernel kernel = SDK::KernelBuilder::make();
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    UserAppEntry();

    return 0;
}

