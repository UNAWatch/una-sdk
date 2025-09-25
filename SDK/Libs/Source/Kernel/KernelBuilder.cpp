/**
 ******************************************************************************
 * @file    KernelBuilder.сpp
 * @date    24-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Helper to construct an SDK::Kernel facade from an IKernel provider.
 * @details Declares a lightweight builder that resolves all required subsystem
 *          interfaces via @c IKernel::queryInterface and returns an initialized
 *          @c SDK::Kernel facade. The @c require<T>() helper asserts that each
 *          requested interface is available and safely casts the returned pointer.
 *
 * @note    This header only declares the builder; the actual construction logic
 *          (e.g., the @c build() definition and how the @c IKernel instance is
 *          obtained) is expected to be provided in the corresponding source file.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Kernel/KernelBuilder.hpp"

/// Global kernel pointer
extern const IKernel* kernel;

SDK::Kernel SDK::KernelBuilder::make()
{
    SDK::Kernel k(require<SDK::Interface::IPower>(kernel,               IKernel::IntfID::IID_POWER),
                  require<SDK::Interface::ISettings>(kernel,            IKernel::IntfID::IID_SETTINGS),
                  require<SDK::Interface::IFileSystem>(kernel,          IKernel::IntfID::IID_FILESYSTEM),
                  kernel->mem,
                  require<SDK::Interface::ISynchManager>(kernel,        IKernel::IntfID::IID_SYNCH_MANAGER),
                  require<SDK::Interface::ISensorManager>(kernel,       IKernel::IntfID::IID_SENSOR_MANAGER),
                  kernel->app,
                  require<SDK::Interface::IServiceControl>(kernel,      IKernel::IntfID::IID_SERVICE_CONTROL),
                  require<SDK::Interface::IGUIControl>(kernel,          IKernel::IntfID::IID_GUI_CONTROL),
                  require<SDK::Interface::IBacklight>(kernel,           IKernel::IntfID::IID_BACKLIGHT),
                  require<SDK::Interface::IVibro>(kernel,               IKernel::IntfID::IID_VIBRO),
                  require<SDK::Interface::IBuzzer>(kernel,              IKernel::IntfID::IID_BUZZER));

    return k;
}
