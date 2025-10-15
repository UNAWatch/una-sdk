/**
 ******************************************************************************
 * @file    IKernelInterfaceProvider.hpp
 * @date    24-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Kernel Interface Provider (IKIP): typed lookup of kernel sub-interfaces.
 *
 * @details IKIP is a minimal, ABI-stable façade that lets user applications obtain
 *          pointers to kernel subsystems at runtime via an enum-based identifier.
 *          The returned pointers are non-owning and remain valid for the lifetime
 *          guaranteed by the kernel. Typical usage:
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
 * @brief Interface to query kernel sub-interfaces by identifier.
 *
 * This interface exposes a single method, @ref queryInterface, which maps a
 * stable integer identifier to a @c void* pointer to the corresponding kernel
 * subsystem interface (e.g., @c IPower, @c ISettings, etc.).
 */
class IKIP {
public:
    /**
     * @brief Stable identifiers for kernel-provided interfaces.
     *
     * The numeric values are part of the ABI and must remain stable across
     * releases. For each ID, @ref queryInterface returns a pointer that can
     * be cast to the documented interface type.
     */
    enum class IntfID : uint32_t {
        IID_SYSTEM              = 0x00010000,   // SDK::Interface::ISystem
        IID_LOGGER              = 0x00020000,   // SDK::Interface::ILogger
        IID_APP_MEMORY          = 0x00030000,   // SDK::Interface::IAppMemory
        IID_APP                 = 0x00040000,   // SDK::Interface::IApp
        IID_APP_CAPABILITIES    = 0x00050000,   // SDK::Interface::IAppCapabilities

        IID_SYNCH_MANAGER       = 0x00060000,   // SDK::Interface::ISynchManager
        IID_SERVICE_CONTROL     = 0x00070000,   // SDK::Interface::IServiceControl
        IID_GUI_CONTROL         = 0x00080000,   // SDK::Interface::IGUIControl

        IID_POWER               = 0x00090000,   // SDK::Interface::IPower
        IID_SETTINGS            = 0x000A0000,   // SDK::Interface::ISettings
        IID_FILESYSTEM          = 0x000B0000,   // SDK::Interface::IFileSystem
        IID_BACKLIGHT           = 0x000C0000,   // SDK::Interface::IBacklight
        IID_VIBRO               = 0x000D0000,   // SDK::Interface::IVibro
        IID_BUZZER              = 0x000E0000,   // SDK::Interface::IBuzzer
        IID_TIME                = 0x000F0000,   // SDK::Interface::ITime

        IID_SENSOR_MANAGER      = 0x00100000,   // SDK::Interface::ISensorManager

        IID_COUNT                               // Number of entries
    };

    /**
     * @brief Query a kernel interface by identifier.
     *
     * @param iid  The interface identifier (see @ref IKIP::IntfID).
     * @return     A non-owning pointer to the requested interface, or @c nullptr
     *             if the interface is unavailable.
     *
     * @warning The caller must cast the returned @c void* to the proper type that
     *          corresponds to @p iid. Passing a mismatched type is undefined behavior.
     * @note    Thread-safety of the returned interface depends on the concrete
     *          implementation; consult the platform documentation.
     */
    virtual void* queryInterface(IntfID iid) const = 0;

protected:
    /**
     * @brief Virtual destructor for interface type.
     */
    virtual ~IKIP() = default;
};

} // namespace SDK::Interface
