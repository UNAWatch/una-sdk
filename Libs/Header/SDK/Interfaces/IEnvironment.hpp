/**
 ******************************************************************************
 * @file    IEnvironment.hpp
 * @date    03-Sempember-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Contains time-related interface definitions.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

namespace SDK::Interface
{

/**
 * @brief Environment interface.
 *
 * Provides access to environment and system-level information.
 * This interface can be extended to include various services such as
 * kernel version, hardware details, or platform-specific parameters.
 */
class IEnvironment {
public:
    /**
     * @brief Get the kernel version.
     *
     * This method returns the current version of the system kernel.
     *
     * @return Kernel version as an unsigned 8-bit integer.
     */
    virtual uint8_t getKernelVersion() = 0;

protected:
    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived classes.
     */
    virtual ~IEnvironment() = default;

};

} // namespace SDK::Interface
