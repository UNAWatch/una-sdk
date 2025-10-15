/**
 ******************************************************************************
 * @file    KernelBuilder.сpp
 * @date    24-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Helper to construct an SDK::Kernel facade from an k provider.
 * @details Declares a lightweight builder that resolves all required subsystem
 *          interfaces via @c k::queryInterface and returns an initialized
 *          @c SDK::Kernel facade. The @c require<T>() helper asserts that each
 *          requested interface is available and safely casts the returned pointer.
 *
 * @note    This header only declares the builder; the actual construction logic
 *          (e.g., the @c build() definition and how the @c k instance is
 *          obtained) is expected to be provided in the corresponding source file.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Kernel/KernelBuilder.hpp"


SDK::Kernel SDK::KernelBuilder::make(const SDK::Interface::IKernel* ikernel)
{
    SDK::Kernel k(require<SDK::Interface::ISystem>(ikernel,            SDK::Interface::IKIP::IntfID::IID_SYSTEM),
                  require<SDK::Interface::ILogger>(ikernel,            SDK::Interface::IKIP::IntfID::IID_LOGGER),
                  require<SDK::Interface::IAppMemory>(ikernel,         SDK::Interface::IKIP::IntfID::IID_APP_MEMORY),
                  require<SDK::Interface::IApp>(ikernel,               SDK::Interface::IKIP::IntfID::IID_APP),
                  require<SDK::Interface::IAppCapabilities>(ikernel,   SDK::Interface::IKIP::IntfID::IID_APP_CAPABILITIES),
                  require<SDK::Interface::ISynchManager>(ikernel,      SDK::Interface::IKIP::IntfID::IID_SYNCH_MANAGER),
                  require<SDK::Interface::IServiceControl>(ikernel,    SDK::Interface::IKIP::IntfID::IID_SERVICE_CONTROL),
                  require<SDK::Interface::IGUIControl>(ikernel,        SDK::Interface::IKIP::IntfID::IID_GUI_CONTROL),
                  require<SDK::Interface::IPower>(ikernel,             SDK::Interface::IKIP::IntfID::IID_POWER),
                  require<SDK::Interface::ISettings>(ikernel,          SDK::Interface::IKIP::IntfID::IID_SETTINGS),
                  require<SDK::Interface::IFileSystem>(ikernel,        SDK::Interface::IKIP::IntfID::IID_FILESYSTEM),
                  require<SDK::Interface::IBacklight>(ikernel,         SDK::Interface::IKIP::IntfID::IID_BACKLIGHT),
                  require<SDK::Interface::IVibro>(ikernel,             SDK::Interface::IKIP::IntfID::IID_VIBRO),
                  require<SDK::Interface::IBuzzer>(ikernel,            SDK::Interface::IKIP::IntfID::IID_BUZZER),
                  require<SDK::Interface::ITime>(ikernel,              SDK::Interface::IKIP::IntfID::IID_TIME),
                  require<SDK::Interface::ISensorManager>(ikernel,     SDK::Interface::IKIP::IntfID::IID_SENSOR_MANAGER));

    return k;
}
