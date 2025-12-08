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


SDK::Kernel SDK::KernelBuilder::make(const SDK::Interface::IKernel *ikernel)
{
    SDK::Kernel k(require<SDK::Interface::ISystem>(ikernel,            SDK::Interface::IKIP::IntfID::IID_SYSTEM),
                  require<SDK::Interface::ILogger>(ikernel,            SDK::Interface::IKIP::IntfID::IID_LOGGER),
                  require<SDK::Interface::IAppMemory>(ikernel,         SDK::Interface::IKIP::IntfID::IID_APP_MEMORY),
                  require<SDK::Interface::IAppComm>(ikernel,           SDK::Interface::IKIP::IntfID::IID_APP_COMM),
                  require<SDK::Interface::IFileSystem>(ikernel,        SDK::Interface::IKIP::IntfID::IID_FILESYSTEM));

    return k;
}
