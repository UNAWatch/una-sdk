/**
 ******************************************************************************
 * @file    IApp.hpp
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

#pragma once

#include <cstdint>

namespace SDK::Interface
{

/**
 * @brief Interface for an user application interacting with the kernel.
 *
 * This interface provides methods for managing the application's lifecycle,
 * handling display output, and processing user input events.
 */
class IApp {
public:

    /**
     * @brief Application frame rate for display updates.
     */
    static constexpr const uint32_t kFrameRate = 10;

    /**
     * @brief Reasons for application launch.
     */
    enum class LaunchReason {
        AUTO_START,
        ON_DEMAND
    };

    /**
     * @brief External button definitions.
     */
    enum Button : uint8_t
    {
        BUTTON_L1   = '1',
        BUTTON_L2   = '2',
        BUTTON_R1   = '3',
        BUTTON_R2   = '4',
        BUTTON_L1R2 = 'z',
    };

protected:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IApp() = default;
};

} // namespace SDK::Interface
