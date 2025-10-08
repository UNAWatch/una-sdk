/**
 ******************************************************************************
 * @file    SvcBootstrap.hpp
 * @date    09-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Application bootstrapper (composition root) for the Service app.
 *
 * @details Provides a thin, one-shot entry point that:
 *          1) obtains the SDK Kernel facade,
 *          2) constructs the user application instance,
 *          3) registers it in kernel.app,
 *          4) finalizes initialization and transfers control to the app's main loop.
 *
 *          Keep this class free of business logic — it should only wire
 *          dependencies and hand off control.
 ******************************************************************************
 */

#pragma once

#include "Service.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

namespace SDK::Service {

/**
 * @class Bootstrap
 * @brief One-shot composition root that wires and runs the Service application.
 *
 * @details On construction it acquires the Kernel and registers the application.
 *          Call @ref run() once to complete initialization and enter the app's
 *          main loop. Returns when @c Service::run() exits.
 *
 * @note    If you need alternative launch modes (e.g., simulator vs device),
 *          create separate bootstrappers or pass configuration into the ctor.
 */
class Bootstrap {
public:
    /**
     * @brief Construct the bootstrapper and register the app in @c kernel.app.
     *
     * Acquires the Kernel facade from @c KernelProviderService and constructs
     * the application instance. The instance is immediately registered as the
     * current app via @c registerApp().
     *
     * @post  @ref mService is registered in @c mKernel.app.
     */
    explicit Bootstrap() : mService()
    {
        auto& kernel = SDK::KernelProviderService::GetInstance().getKernel();

        kernel.app.registerApp(&mService);
    }

    /**
     * @brief Default destructor.
     *
     * Place optional cleanup here if your API later supports explicit
     * unregistration or additional teardown steps.
     */
    ~Bootstrap() = default;

    /**
     * @brief Finalize initialization and enter the application's main loop.
     *
     * Calls @c mKernel.app.initialized() to signal readiness and then invokes
     * @c mService.run(). This call blocks until the application loop returns.
     */
    void run()
    {
        auto& kernel = SDK::KernelProviderService::GetInstance().getKernel();

        kernel.app.initialized();

        mService.run();

        exit(0);    // Exit from app in correct way
        // The following callbacks will be triggered in sequence: 
        // 'onPause() -> onStop() -> onDestroy()'.
    }

private:
    ::Service mService; ///< User application instance registered with @c kernel.app.
};

} // namespace SDK::Service
