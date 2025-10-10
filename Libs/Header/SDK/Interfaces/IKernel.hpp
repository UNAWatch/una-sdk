/**
 ******************************************************************************
 * @file    IKernel.hpp
 * @date    24-September-2022
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Kernel interface for dynamic service discovery.
 * @details This header defines the @c IKernel interface used to obtain pointers
 *          to subsystem interfaces (e.g., power, settings, filesystem) at run time.
 *          Services are addressed via the @ref IKernel::IntfID enumeration and
 *          retrieved through a COM-style @ref IKernel::queryInterface method.
 *
 * @note    Returned pointers are borrowed (non-owning) and remain valid as long
 *          as the underlying kernel instance and the corresponding service exist.
 *          Callers must cast the returned @c void* back to the requested interface
 *          type that matches the provided @ref IKernel::IntfID.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <cstddef>

#include "SDK/Interfaces/IKernelIntfProvider.hpp"

#include "SDK/Interfaces/ISystem.hpp"
#include "SDK/Interfaces/ILogger.hpp"
#include "SDK/Interfaces/IAppMemory.hpp"
#include "SDK/Interfaces/IApp.hpp"
#include "SDK/Interfaces/IAppCapabilities.hpp"

#include "SDK/Interfaces/ISynchManager.hpp"
#include "SDK/Interfaces/IAppControl.hpp"

#include "SDK/Interfaces/IPower.hpp"
#include "SDK/Interfaces/ISettings.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Interfaces/IBacklight.hpp"
#include "SDK/Interfaces/IVibro.hpp"
#include "SDK/Interfaces/IBuzzer.hpp"
#include "SDK/Interfaces/ITime.hpp"

#include "SDK/Interfaces/ISensorManager.hpp"

namespace SDK::Interface
{

#define DUMMY_KERNEL_ADDR           (0xA5A5A5A5)
#define KERNEL_INTERFACE_VERSION    (1)

class IKernel {
public:

    IKernel(SDK::Interface::IKIP& kip) : kip(kip)
    {}

    virtual ~IKernel() = default;

    uint32_t version = KERNEL_INTERFACE_VERSION;

    SDK::Interface::IKIP& kip;
};

} // namespace SDK::Interface
