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

#ifndef __INTERFACE_I_KERNEL_HPP
#define __INTERFACE_I_KERNEL_HPP

#include <cstdint>
#include <cstddef>
#include <cassert>

#include "SDK/Interfaces/IPower.hpp"
#include "SDK/Interfaces/ISettings.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Interfaces/IUserAppMemAllocator.hpp"
#include "SDK/Interfaces/ISynchManager.hpp"
#include "SDK/Interfaces/ISensorManager.hpp"
#include "SDK/Interfaces/IUserApp.hpp"
#include "SDK/Interfaces/IUserAppControl.hpp"
#include "SDK/Interfaces/IBacklight.hpp"
#include "SDK/Interfaces/IVibro.hpp"
#include "SDK/Interfaces/IBuzzer.hpp"

#define DUMMY_KERNEL_ADDR           (0xA5A5A5A5)
#define KERNEL_INTERFACE_VERSION    (0)

class IKernel {
public:
    enum class IntfID : uint32_t {
        IID_POWER = 0,          // SDK::Interface::IPower
        IID_SETTINGS,           // SDK::Interface::ISettings
        IID_FILESYSTEM,         // SDK::Interface::IFileSystem
        IID_MEM_ALLOCATOR,      // SDK::Interface::IUserAppMemAllocator
        IID_SYNCH_MANAGER,      // SDK::Interface::ISynchManager
        IID_SENSOR_MANAGER,     // SDK::Interface::ISensorManager
        IID_USER_APP,           // SDK::Interface::IUserApp
        IID_SERVICE_CONTROL,    // SDK::Interface::IServiceControl
        IID_GUI_CONTROL,        // SDK::Interface::IGUIControl
        IID_BACKLIGHT,          // SDK::Interface::IBacklight
        IID_VIBRO,              // SDK::Interface::IVibro
        IID_BUZZER,             // SDK::Interface::IBuzzer
        IID_COUNT               // Number of entries
    };

    virtual ~IKernel() = default;

    uint32_t version = KERNEL_INTERFACE_VERSION;

    virtual void* queryInterface(IntfID iid) const = 0;
};

#endif /* __INTERFACE_I_KERNEL_HPP */
