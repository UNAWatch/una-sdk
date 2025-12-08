/**
 ******************************************************************************
 * @file    main.сpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   The GUI application entry point
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGui.hpp"
#include "SDK/UnaLogger/Logger.h"

// Include path must be defined globally
#include "Gui.hpp"

/**
 * @brief  Global kernel pointer defined in system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/*
 * @brief Main entry point for an application GUI.
 * @retval int
 */
int main()
{
    // Create Kernel instance
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);

    // Initialize KernelProvider to access to the Kernel from everywhere
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    // Initialize application logger
    Logger_init(kernel.log);

    /**
     * @brief Initializes and runs the GUI using static storage duration.
     *
     * A function-local static variable allocates the Gui instance in static
     * memory, but its constructor is executed only upon the first entry into this
     * block — ensuring initialization happens strictly after base initialization.
     *
     * The object remains alive until program termination, so its destructor is
     * automatically invoked after main() returns, providing proper shutdown.
     *
     * This approach avoids a global singleton while guaranteeing static allocation
     * without using stack or dynamic memory.
     */
    {
        static Gui gui(kernel);
        gui.run();  // User application entry point
    }

    return 0;
}

