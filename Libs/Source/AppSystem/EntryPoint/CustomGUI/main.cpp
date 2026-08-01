/**
 ******************************************************************************
 * @file    main.cpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko
 *          <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   GUI application entry point.
 *
 * This module defines the controlled startup sequence for the GUI part of
 * the application. The Gui instance is constructed manually in statically
 * allocated storage to guarantee precise initialization order and predictable
 * destruction semantics in environments where automatic C++ static object
 * teardown may be disabled or unavailable.
 *
 ******************************************************************************
 */

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/UnaLogger/Logger.h"

// Include path must be defined globally
#include "Gui.hpp"

/**
 * @brief Raw statically allocated storage for the Gui instance.
 *
 * This buffer uses static storage duration and is aligned to match the
 * requirements of Gui. It allows constructing the Gui object using
 * placement-new at a precisely controlled moment (after kernel and logger
 * initialization), while avoiding stack allocation or heap usage.
 *
 * Manual static storage is used because embedded toolchains may not reliably
 * support automatic static C++ object destruction via .fini_array or
 * __cxa_atexit. This guarantees that the object's lifetime is fully controlled
 * and independent of the CRT.
 */
alignas(Gui) static uint8_t sGuiStorage[sizeof(Gui)];

/**
 * @brief Pointer to the Gui instance placed in @ref sGuiStorage.
 *
 * The pointer is assigned after performing placement-new and reset after
 * the manual destructor call. This explicit lifecycle management allows
 * deterministic construction and destruction of the Gui instance, even on
 * platforms where automatic static destructors are not supported.
 */
static Gui* sGui = nullptr;

/**
 * @brief Global kernel pointer provided by system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/**
 * @brief Main entry point for the GUI application.
 *
 * Startup sequence:
 *   1. Construct Kernel instance from platform-provided IKernel.
 *   2. Register KernelProviderGUI for global kernel access.
 *   3. Initialize the logging subsystem.
 *   4. Manually construct Gui object in static storage using placement-new.
 *   5. Run the GUI application (typically blocking).
 *   6. Explicitly invoke the Gui destructor and clear the pointer.
 *
 * This explicit construction/destruction pattern ensures:
 *  - deterministic initialization order,
 *  - no dependency on CRT-managed static destructors,
 *  - no runtime heap allocation,
 *  - GUI initialization strictly after kernel and logger setup,
 *  - compatibility with freestanding embedded environments.
 *
 * @retval int Exit code.
 */
int main()
{
    // Create Kernel instance
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);

    // Initialize KernelProvider to allow global kernel access
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    // Initialize application logger
    Logger_init(kernel.log);

    // Construct the Gui instance in static storage
    sGui = new (sGuiStorage) Gui(kernel);

    // Run the GUI application (user entry point)
    sGui->run();

    // Manually destroy the Gui instance
    sGui->~Gui();
    sGui = nullptr;

    return 0;
}
