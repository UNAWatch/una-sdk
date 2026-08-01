/**
 ******************************************************************************
 * @file    main.cpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko
 *          <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SERVICE application entry point.
 *
 * This module defines the controlled startup sequence for the Service
 * application. The Service instance is constructed manually in statically
 * allocated storage to guarantee precise initialization order and predictable
 * destruction semantics in environments where automatic C++ static object
 * teardown may be disabled or unavailable.
 *
 ******************************************************************************
 */

#include <new>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Kernel/KernelBuilder.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/UnaLogger/Logger.h"

// Include path must be defined globally
#include "Service.hpp"

/**
 * @brief Raw statically allocated storage for the Service instance.
 *
 * This buffer uses static storage duration and is aligned to match the
 * requirements of Service. It allows constructing the Service object using
 * placement-new at a precisely controlled moment (after logger and kernel
 * initialization), while avoiding stack allocation or heap usage.
 *
 * Manual static storage is used here because embedded toolchains may not
 * reliably support automatic static C++ object destruction via .fini_array
 * or __cxa_atexit. This strategy guarantees that the object's lifetime is
 * explicitly managed by the application.
 */
alignas(Service) static uint8_t sServiceStorage[sizeof(Service)];

/**
 * @brief Pointer to the Service instance placed in @ref sServiceStorage.
 *
 * The pointer is assigned after performing placement-new and reset after
 * the manual destructor call. Keeping the pointer separate from raw storage
 * provides clarity and prevents misuse when the object is not active.
 */
static Service* sService = nullptr;

/**
 * @brief Global kernel pointer provided by system.cpp.
 */
extern const SDK::Interface::IKernel* gIKernel;

/**
 * @brief Main entry point for the Service application.
 *
 * Startup sequence:
 *   1. Construct Kernel instance from platform-provided IKernel.
 *   2. Register KernelProviderService for global kernel access.
 *   3. Initialize the logging subsystem.
 *   4. Manually construct Service object in static storage using placement-new.
 *   5. Run the service.
 *   6. Explicitly invoke the Service destructor and clear the pointer.
 *
 * This explicit construction/destruction pattern ensures:
 *  - deterministic initialization order,
 *  - no dependency on CRT-managed static destructors,
 *  - no runtime heap allocation,
 *  - no premature object construction before logger setup,
 *  - compatibility with freestanding embedded environments.
 *
 * @retval int Exit code.
 */
int main()
{
    // Create Kernel instance
    SDK::Kernel kernel = SDK::KernelBuilder::make(gIKernel);

    // Initialize KernelProvider to allow global kernel access
    SDK::KernelProviderService::CreateInstance(&kernel);

    // Initialize application logger
    Logger_init(kernel.log);

    // Construct the Service instance in static storage
    sService = new (sServiceStorage) Service(kernel);

    // Run the service (typically blocking)
    sService->run();

    // Manually destroy the Service instance
    sService->~Service();
    sService = nullptr;

    return 0;
}
