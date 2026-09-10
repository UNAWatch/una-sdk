/**
 ******************************************************************************
 * @file    main.cpp
 * @brief   Entry point for a GUI process built on LVGL.
 *
 * Mirrors EntryPoint/TouchGFX/main.cpp: bind the kernel, publish it through
 * KernelProviderGUI, start the logger, then hand over to the toolkit. The app
 * supplies una_lvgl_app_init(), which builds its model and first screen; the
 * port's frame loop then runs until the kernel stops the process.
 ******************************************************************************
 */

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/UnaLogger/Logger.h"
#include "SDK/Port/LVGL/LvglPort.hpp"

/**
 * @brief Global kernel pointer provided by system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/**
 * @brief Application hook: create the model and load the first screen.
 *
 * Called once, after LVGL and the display are initialised and before the
 * first frame. Defined by the application.
 */
extern "C" void una_lvgl_app_init(void);

int main()
{
    // Create Kernel instance
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);

    // Initialize KernelProvider to allow global kernel access
    SDK::KernelProviderGUI::CreateInstance(&kernel);

    // Initialize application logger
    Logger_init(kernel.log);

    SDK::LVGL::Port& port = SDK::LVGL::Port::GetInstance();
    port.init();
    una_lvgl_app_init();
    port.run();   // No return

    return 0;
}
