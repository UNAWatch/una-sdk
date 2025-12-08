/**
 ******************************************************************************
 * @file    Kernel.hpp
 * @date    24-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Client-side Kernel facade that binds interface references via IKernel::queryInterface.
 * @details This wrapper resolves all required subsystem interfaces from a provided @c IKernel
 *          instance and exposes them as references in a fixed, well-known order. Resolution is
 *          performed once in the constructor; each field then aliases the underlying service.
 *
 * @note    All references are borrowed (non-owning) and remain valid only while the source
 *          @c IKernel and the corresponding services are alive.
 * @note    Construction @b asserts on missing interfaces. Replace @c assert with your error
 *          handling if needed.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cassert>

#include "SDK/Interfaces/IKernel.hpp"

namespace SDK {

/**
 * @brief Aggregate of kernel service interfaces (non-owning references).
 *
 * @note This facade does not perform any validation or lifetime management.
 *       The caller must ensure that referenced interfaces outlive this object.
 */
class Kernel {
public:
    /**
     * @brief Construct the facade by binding kernel service references.
     *
     * @param sys       Reference to system interface.
     * @param log       Reference to logger interface.
     * @param mem       Reference to user-app memory allocator.
     * @param comm      Reference to user-app communication interface.
     * @param fs        Reference to filesystem interface.
     */
    Kernel(SDK::Interface::ISystem      &sys,
           SDK::Interface::ILogger      &log,
           SDK::Interface::IAppMemory   &mem,
           SDK::Interface::IAppComm     &comm,
           SDK::Interface::IFileSystem  &fs)
    : sys(sys)
    , log(log)
    , mem(mem)
    , comm(comm)
    , fs(fs)
    {}

    virtual ~Kernel() = default;

    SDK::Interface::ISystem     &sys;
    SDK::Interface::ILogger     &log;
    SDK::Interface::IAppMemory  &mem;
    SDK::Interface::IAppComm    &comm;
    SDK::Interface::IFileSystem &fs;
};

} // namespace SDK
