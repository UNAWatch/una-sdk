/**
 ******************************************************************************
 * @file    IKernel.hpp
 * @date    06-12-2025
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

#include "SDK/Interfaces/IKIP.hpp"
#include "SDK/Interfaces/ISystem.hpp"
#include "SDK/Interfaces/ILogger.hpp"
#include "SDK/Interfaces/IAppMemory.hpp"
#include "SDK/Interfaces/IAppComm.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

namespace SDK::Interface
{

#define DUMMY_KERNEL_ADDR           (0xA5A5A5A5)
#define KERNEL_INTERFACE_VERSION    (2)

class IKernel {
public:
    IKernel(IKIP& kip) : kip(kip)
    {}

    virtual ~IKernel() = default;

    uint32_t version = KERNEL_INTERFACE_VERSION;

    IKIP& kip;
};

} // namespace SDK::Interface
