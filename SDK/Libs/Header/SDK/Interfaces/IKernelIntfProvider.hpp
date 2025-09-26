/**
 ******************************************************************************
 * @file    IUserApp.hpp.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for a user application interacting with the kernel.
 *
 * This interface defines the lifecycle management and essential interactions
 * between the user application and the kernel, including display updates
 * input handling and generates notifications.
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
 * @brief Interface for an user application interacting with the kernel.
 *
 * This interface provides methods for managing the application's lifecycle,
 * handling display output, and processing user input events.
 */
class IKIP {
public:
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

    virtual void* queryInterface(IntfID iid) const = 0;

protected:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IKIP() = default;
};

} /* namespace Interface */

#endif /* __INTERFACE_I_USER_APP_HPP */
