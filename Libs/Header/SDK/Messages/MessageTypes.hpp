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
 * Example custom type:
 *   constexpr MessageType::Type MY_CUSTOM_EVENT = 0x0100;
 *   constexpr MessageType::Type MY_CUSTOM_REQUEST = 0x1100;
 */
namespace MessageType {
    // Type alias for message type values
    using Type = uint32_t;

    // =========================================================================
    // Events: 0x0000 - 0x0FFF (fire-and-forget, kernel -> app)
    // =========================================================================

    // System events
    constexpr Type EVENT_SYSTEM_TIME           = 0x0001;
    constexpr Type EVENT_SYSTEM_POWER_LOW      = 0x0002;
    constexpr Type EVENT_SYSTEM_POWER_CRITICAL = 0x0003;

    // User input events
    constexpr Type EVENT_BUTTON_PRESS          = 0x0010;
    constexpr Type EVENT_BUTTON_RELEASE        = 0x0011;
    constexpr Type EVENT_BUTTON_LONG_PRESS     = 0x0012;
    constexpr Type EVENT_TOUCH_TAP             = 0x0013;
    constexpr Type EVENT_TOUCH_SWIPE           = 0x0014;

    // Sensor events
    constexpr Type EVENT_SENSOR_ACCELEROMETER  = 0x0020;
    constexpr Type EVENT_SENSOR_GYROSCOPE      = 0x0021;
    constexpr Type EVENT_SENSOR_MAGNETOMETER   = 0x0022;
    constexpr Type EVENT_SENSOR_HEARTRATE      = 0x0023;
    constexpr Type EVENT_SENSOR_SPO2           = 0x0024;

    // =========================================================================
    // Requests: 0x1000 - 0x1FFF (app -> kernel, response expected)
    // =========================================================================

    // Position/navigation requests
    constexpr Type REQUEST_GPS_LOCATION        = 0x1010;
    constexpr Type REQUEST_GPS_START           = 0x1011;
    constexpr Type REQUEST_GPS_STOP            = 0x1012;

    // System information requests
    constexpr Type REQUEST_BATTERY_STATUS      = 0x1002;
    constexpr Type REQUEST_SYSTEM_INFO         = 0x1003;
    constexpr Type REQUEST_MEMORY_INFO         = 0x1004;

    // Application control requests (Service -> Kernel)
    constexpr Type REQUEST_RUN_GUI             = 0x1020;  // Request kernel to launch GUI

    // Display requests
    constexpr Type REQUEST_DISPLAY_UPDATE      = 0x1030;
    constexpr Type REQUEST_DISPLAY_BRIGHTNESS  = 0x1031;

    // Storage requests
    constexpr Type REQUEST_FILE_READ           = 0x1040;
    constexpr Type REQUEST_FILE_WRITE          = 0x1041;
    constexpr Type REQUEST_FILE_DELETE         = 0x1042;

    // Bluetooth requests
    constexpr Type REQUEST_BLE_CONNECT         = 0x1050;
    constexpr Type REQUEST_BLE_DISCONNECT      = 0x1051;
    constexpr Type REQUEST_BLE_SEND_DATA       = 0x1052;

    // =========================================================================
    // Commands: 0x3000 - 0x3FFF (kernel -> app)
    // =========================================================================

    // Application lifecycle commands (response expected)
    constexpr Type COMMAND_APP_RUN             = 0x3001;
    constexpr Type COMMAND_APP_STOP            = 0x3002;

    // Kernel -> Service to notify about GUI state
    constexpr Type COMMAND_APP_NOTIF_GUI_RUN   = 0x3003;
    constexpr Type COMMAND_APP_NOTIF_GUI_STOP  = 0x3004;

    // Kernel -> GUI
    constexpr Type COMMAND_APP_GUI_SUSPEND     = 0x3005;
    constexpr Type COMMAND_APP_GUI_RESUME      = 0x3006;

    constexpr Type REQUEST_APP_TERMINATE       = 0x3007;

    constexpr Type EVENT_GUI_FRAME_TICK        = 0x3010;

    // =========================================================================
    // Range Constants (for validation and custom type allocation)
    // =========================================================================

    constexpr Type RANGE_EVENT_MIN             = 0x00000000;
    constexpr Type RANGE_EVENT_MAX             = 0x00000FFF;

    constexpr Type RANGE_REQUEST_MIN           = 0x00001000;
    constexpr Type RANGE_REQUEST_MAX           = 0x00001FFF;

    constexpr Type RANGE_RESPONSE_MIN          = 0x00002000;
    constexpr Type RANGE_RESPONSE_MAX          = 0x00002FFF;

    constexpr Type RANGE_COMMAND_MIN           = 0x00003000;
    constexpr Type RANGE_COMMAND_MAX           = 0x00003FFF;

    constexpr Type RANGE_CUSTOM_MIN            = 0x00004000;
    constexpr Type RANGE_CUSTOM_MAX            = 0x00007FFF;

    // Internal application messages (Service <-> GUI direct, used by DualAppComm)
    constexpr Type RANGE_INTERNAL_APP_MIN      = 0x00008000;
    constexpr Type RANGE_INTERNAL_APP_MAX      = 0x00008FFF;

    // Application-specific custom messages (Service <-> GUI direct, used by DualAppComm)
    constexpr Type RANGE_APP_SPECIFIC_MIN      = 0x00009000;
    constexpr Type RANGE_APP_SPECIFIC_MAX      = 0x00FFFFFF;

    // Internal kernel messages (kernel subsystems only)
    constexpr Type RANGE_INTERNAL_KERNEL_MIN   = 0x01000000;
    constexpr Type RANGE_INTERNAL_KERNEL_MAX   = 0xFFFFFFFF;
}

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Check if message type is an event (fire-and-forget)
 * @param type Message type to check
 * @retval true if type is in event range
 */
inline bool isEvent(MessageType::Type type) {
    return (type >= MessageType::RANGE_EVENT_MIN &&
            type <= MessageType::RANGE_EVENT_MAX);
}

/**
 * @brief Check if message type is a request (expects response)
 * @param type Message type to check
 * @retval true if type is in request range
 */
inline bool isRequest(MessageType::Type type) {
    return (type >= MessageType::RANGE_REQUEST_MIN &&
            type <= MessageType::RANGE_REQUEST_MAX);
}

/**
 * @brief Check if message type is a command (expects response)
 * @param type Message type to check
 * @retval true if type is in command range
 */
inline bool isCommand(MessageType::Type type) {
    return (type >= MessageType::RANGE_COMMAND_MIN &&
            type <= MessageType::RANGE_COMMAND_MAX);
}

/**
 * @brief Check if message type expects a response
 * @param type Message type to check
 * @retval true if type is request or command
 */
inline bool needsResponse(MessageType::Type type) {
    return isRequest(type) || isCommand(type);
}

/**
 * @brief Check if message type is internal kernel message
 * @param type Message type to check
 * @retval true if type is in internal kernel range (>= 0x0100_0000)
 * @note Internal messages cannot be sent/received by applications
 */
inline bool isInternalKernelMessage(MessageType::Type type) {
    return (type >= MessageType::RANGE_INTERNAL_KERNEL_MIN);
}

/**
 * @brief Check if message type is internal application message
 * @param type Message type to check
 * @retval true if type is in application-internal range
 */
inline bool isApplicationInternalMessage(MessageType::Type type) {
    return (type >= MessageType::RANGE_INTERNAL_APP_MIN &&
            type <= MessageType::RANGE_INTERNAL_APP_MAX);
}

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
 * @brief Check if message type is internal for applicatio (Service <-> GUI)
 * @param type Message type to check
 * @retval true if internal message (direct routing)
 * @retval false if regular message (goes through kernel)
 */
inline bool isApplicationMessage(MessageType::Type type) {
    return (isApplicationInternalMessage(type) ||
            isApplicationSpecificMessage(type));
}

} // namespace SDK
