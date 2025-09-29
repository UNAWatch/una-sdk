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

#ifndef __INTERFACE_I_KERNEL_INTERFACE_PROVIDER_HPP
#define __INTERFACE_I_KERNEL_INTERFACE_PROVIDER_HPP

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
        IID_POWER = 0,          // SDK::Interface::IPower
        IID_SETTINGS,           // SDK::Interface::ISettings
        IID_FILESYSTEM,         // SDK::Interface::IFileSystem
        IID_SYNCH_MANAGER,      // SDK::Interface::ISynchManager
        IID_SENSOR_MANAGER,     // SDK::Interface::ISensorManager
        IID_SERVICE_CONTROL,    // SDK::Interface::IServiceControl
        IID_GUI_CONTROL,        // SDK::Interface::IGUIControl
        IID_BACKLIGHT,          // SDK::Interface::IBacklight
        IID_VIBRO,              // SDK::Interface::IVibro
        IID_BUZZER,             // SDK::Interface::IBuzzer
        IID_COUNT               // Number of entries
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

} /* namespace SDK::Interface */

#endif /* __INTERFACE_I_KERNEL_INTERFACE_PROVIDER_HPP */
