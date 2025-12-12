/**
 * @file    MessageTypes.hpp
 * @date    03-12-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Message type definitions for kernel-application communication
 *
 * This file defines common message types used in the system.
 * Applications can define custom types without modifying this file.
 *
 * Type ranges:
 * 0x0000_0000-0x0000_0FFF : Events (fire-and-forget, kernel -> app)
 * 0x0000_1000-0x0000_1FFF : Requests (app -> kernel, response expected)
 * 0x0000_2000-0x0000_2FFF : Responses (kernel -> app, response to request)
 * 0x0000_3000-0x0000_3FFF : Commands (kernel -> app, response expected)
 * 0x0000_4000-0x00FF_FFFF : Application-specific custom messages
 * 0x0100_0000-0xFFFF_FFFF : Internal kernel messages (private)
 *
 * Shared between kernel and applications.
 */

#pragma once

#include <cstdint>


namespace SDK
{

/**
 * @brief Message type namespace
 *
 * Using namespace with constexpr constants instead of enum class
 * for maximum flexibility. Applications can define custom types
 * without explicit casting.
 *
 */
namespace MessageType {
    // Type alias for message type values
    using Type = uint32_t;


    // Application lifecycle commands (response expected)
    constexpr Type COMMAND_APP_RUN              = 0x01010000;
    constexpr Type COMMAND_APP_STOP             = 0x01020000;

    // Kernel -> GUI
    constexpr Type COMMAND_APP_GUI_RESUME       = 0x01030000;
    constexpr Type COMMAND_APP_GUI_SUSPEND      = 0x01040000;

    // Kernel -> Service to notify about GUI state
    constexpr Type COMMAND_APP_NOTIF_GUI_RUN    = 0x01050000;
    constexpr Type COMMAND_APP_NOTIF_GUI_STOP   = 0x01060000;

    // =========================================================================
    // Range Constants (for validation and custom type allocation)
    // =========================================================================

    // Application control requests (app -> Kernel)
    constexpr Type REQUEST_APP_RUN_GUI          = 0x01070000;  // Request kernel to launch GUI
    constexpr Type REQUEST_APP_TERMINATE        = 0x01080000;


    constexpr Type REQUEST_SET_CAPABILITIES     = 0x02010000; // Set app capabilities

    // System info
    constexpr Type REQUEST_BATTERY_STATUS       = 0x02020000; // Get battery level/status
    constexpr Type REQUEST_SYSTEM_SETTINGS      = 0x02030000; // Get system settings
    constexpr Type REQUEST_SYSTEM_INFO          = 0x02040000; // Get system information
    constexpr Type REQUEST_MEMORY_INFO          = 0x02050000; // Get memory statistics


    // Display control (GUI only)
    constexpr Type REQUEST_DISPLAY_CONFIG       = 0x02060000; // Get display configuration
    constexpr Type REQUEST_DISPLAY_UPDATE       = 0x02070000; // Update display buffer

    // Backlight/Audio/Haptic control
    constexpr Type REQUEST_BACKLIGHT_SET        = 0x02080000; // Set backlight brightness/timeout
    constexpr Type REQUEST_BUZZER_PLAY          = 0x02090000; // Play buzzer pattern
    constexpr Type REQUEST_VIBRO_PLAY           = 0x020A0000; // Play vibro pattern


    // System events
    constexpr Type EVENT_SYSTEM_POWER_LOW       = 0x03010000;
    constexpr Type EVENT_SYSTEM_POWER_CRITICAL  = 0x03020000;
    constexpr Type EVENT_GUI_TICK               = 0x03030000;
    constexpr Type EVENT_BUTTON                 = 0x03040000;


    // Glances (Service only)
    constexpr Type REQUEST_GLANCE_CONFIG        = 0x030A0000; // Get available glance area
    constexpr Type REQUEST_GLANCE_UPDATE        = 0x030B0000; // Update glance content
    // Glance events
    constexpr Type EVENT_GLANCE_START           = 0x030C0000;
    constexpr Type EVENT_GLANCE_TICK            = 0x030D0000;
    constexpr Type EVENT_GLANCE_STOP            = 0x030E0000;

    // Sensor Layer (Service only)
    constexpr Type REQUEST_SENSOR_LAYER_GET_DEFAULT    = 0x03100000;
    constexpr Type REQUEST_SENSOR_LAYER_GET_LIST       = 0x03110000;
    constexpr Type REQUEST_SENSOR_LAYER_GET_DESCRIPTOR = 0x03120000;
    constexpr Type REQUEST_SENSOR_LAYER_CONNECT        = 0x03130000;
    constexpr Type REQUEST_SENSOR_LAYER_DISCONNECT     = 0x03140000;
    // Sensor Layer data
    constexpr Type EVENT_SENSOR_LAYER_DATA             = 0x03180000;

    // Application-specific custom messages (Service <-> GUI direct, used by DualAppComm)
    constexpr Type RANGE_APP_SPECIFIC_MIN       = 0x00000000;
    constexpr Type RANGE_APP_SPECIFIC_MAX       = 0x0000FFFF;

    // Internal kernel messages (kernel subsystems only)
    constexpr Type RANGE_INTERNAL_KERNEL_MIN    = 0x80000000;
    constexpr Type RANGE_INTERNAL_KERNEL_MAX    = 0xFFFF0000;

}

// =============================================================================
// Helper Functions
// =============================================================================


/**
 * @brief Check if message type is application custom message
 * @param type Message type to check
 * @retval true if type is in application-specific range
 */
inline bool isApplicationSpecificMessage(MessageType::Type type) {
    return (type >= MessageType::RANGE_APP_SPECIFIC_MIN &&
            type <= MessageType::RANGE_APP_SPECIFIC_MAX);
}

/**
 * @brief Check if message type is internal kernel message
 * @param type Message type to check
 * @retval true if type is in internal kernel range
 * @note Internal messages cannot be sent/received by applications
 */
inline bool isInternalKernelMessage(MessageType::Type type) {
    return (type >= MessageType::RANGE_INTERNAL_KERNEL_MIN &&
            type <= MessageType::RANGE_INTERNAL_KERNEL_MAX);
}

} // namespace SDK
