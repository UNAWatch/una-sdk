/**
 * @file ConcreteMessages.hpp
 * @date 02-12-2024
 * @author Kernel Communication Module
 * @brief Concrete message structures for kernel-application communication
 *
 * This file defines specific message types used in the system.
 * Each message inherits from MessageBase and adds payload fields.
 *
 * Shared between kernel and applications.
 *
 * Message design guidelines:
 * - Keep messages small (prefer multiple small messages over one large)
 * - For request-response: include both request and response fields
 * - Use fixed-size types (uint32_t, float, etc.) for portability
 * - Avoid pointers in message payload
 * - Consider alignment and padding
 */

#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"


namespace SDK
{

// =============================================================================
// Event Messages (fire-and-forget, kernel -> app)
// =============================================================================

/**
 * @brief Button event message
 *
 * Sent by kernel when button state changes.
 * Fire-and-forget event (no response expected).
 */
struct ButtonEventMsg : public MessageBase {
    ButtonEventMsg()
        : MessageBase(MessageType::EVENT_BUTTON_PRESS)
        , buttonId(0)
        , pressed(false)
        , timestamp(0)
    {}

    uint8_t buttonId;       // Button identifier (0-255)
    bool pressed;           // True if pressed, false if released
    uint32_t timestamp;     // Event timestamp in milliseconds
};

/**
 * @brief Sensor data event message
 *
 * Sent by kernel when sensor data available.
 * Contains accelerometer readings.
 */
struct SensorEventMsg : public MessageBase {
    SensorEventMsg()
        : MessageBase(MessageType::EVENT_SENSOR_ACCELEROMETER)
        , sensorId(0)
        , timestamp(0)
        , accelX(0.0f)
        , accelY(0.0f)
        , accelZ(0.0f)
    {}

    uint8_t sensorId;       // Sensor identifier
    uint32_t timestamp;     // Measurement timestamp
    float accelX;           // X-axis acceleration (m/s^2)
    float accelY;           // Y-axis acceleration (m/s^2)
    float accelZ;           // Z-axis acceleration (m/s^2)
};

/**
 * @brief System time event message
 *
 * Sent by kernel on time updates (e.g. RTC sync, timezone change).
 */
struct SystemTimeEventMsg : public MessageBase {
    SystemTimeEventMsg()
        : MessageBase(MessageType::EVENT_SYSTEM_TIME)
        , unixTimestamp(0)
        , timezoneOffset(0)
    {}

    uint64_t unixTimestamp; // Unix timestamp (seconds since epoch)
    int16_t timezoneOffset; // Timezone offset in minutes
};

// =============================================================================
// Request Messages (app -> kernel, response expected)
// =============================================================================

/**
 * @brief GPS location request message
 *
 * Application requests current GPS position from kernel.
 * Kernel fills response fields and signals completion.
 */
struct GpsRequestMsg : public MessageBase {
    // Response fields (filled by kernel)
    float latitude;         // Latitude in degrees (-90 to +90)
    float longitude;        // Longitude in degrees (-180 to +180)
    float altitude;         // Altitude in meters above sea level
    float accuracy;         // Position accuracy in meters
    bool fixAvailable;      // True if valid GPS fix

    GpsRequestMsg()
        : MessageBase(MessageType::REQUEST_GPS_LOCATION)
        , latitude(0.0f)
        , longitude(0.0f)
        , altitude(0.0f)
        , accuracy(0.0f)
        , fixAvailable(false)
    {}
};

/**
 * @brief Battery status request message
 *
 * Application requests current battery information.
 */
struct BatteryRequestMsg : public MessageBase {
    // Response fields (filled by kernel)
    uint8_t batteryLevel;   // Battery level 0-100%
    bool isCharging;        // True if currently charging
    float voltage;          // Battery voltage in volts

    BatteryRequestMsg()
        : MessageBase(MessageType::REQUEST_BATTERY_STATUS)
        , batteryLevel(0)
        , isCharging(false)
        , voltage(0.0f)
    {}
};

/**
 * @brief Run GUI request message
 *
 * Service sends this request when it needs to show information to user.
 * Kernel will launch GUI part of the application if not already running.
 * Response indicates whether GUI was successfully started.
 *
 * Use case: Background service detects important event and needs to
 * notify user by showing GUI interface.
 *
 * Example:
 * @code
 * // Service detects incoming call
 * auto* req = srvComm->allocateMessage<RunGuiRequest>();
 * if (srvComm->sendMessage(req, 1000)) {
 *     if (req->getResult() == MessageResult::SUCCESS) {
 *         // GUI is now running, can send internal messages
 *     }
 * }
 * srvComm->releaseMessage(req);
 * @endcode
 */
struct RunGuiRequest : public MessageBase {
    RunGuiRequest()
        : MessageBase(MessageType::REQUEST_RUN_GUI)
    {
    }
};

/**
 * @brief Display update request message
 *
 * Application requests display content update.
 * Contains display buffer information.
 */
struct DisplayUpdateRequestMsg : public MessageBase {
    DisplayUpdateRequestMsg()
        : MessageBase(MessageType::REQUEST_DISPLAY_UPDATE)
        , x(0)
        , y(0)
        , width(0)
        , height(0)
        , bufferPtr(0)
        , updateComplete(false)
    {}

    // Request fields (filled by application)
    uint16_t x;             // Update region X coordinate
    uint16_t y;             // Update region Y coordinate
    uint16_t width;         // Update region width
    uint16_t height;        // Update region height
    uint32_t bufferPtr;     // Pointer to pixel buffer (as uint32_t)

    // Response fields (filled by kernel)
    bool updateComplete;    // True if update successful
};

// =============================================================================
// Command Messages (kernel -> app, response expected)
// =============================================================================

///**
// * @brief Application status command message
// *
// * Kernel requests status information from application.
// * Application fills response fields and signals completion.
// */
//struct AppStatusCommandMsg : public MessageBase {
//    AppStatusCommandMsg()
//        : MessageBase(MessageType::COMMAND_APP_STATUS)
//        , isRunning(false)
//        , memoryUsedBytes(0)
//        , cpuUsagePercent(0)
//        , errorCount(0)
//    {}
//
//    // Response fields (filled by application)
//    bool isRunning;             // True if application running normally
//    uint32_t memoryUsedBytes;   // Memory usage in bytes
//    uint8_t cpuUsagePercent;    // CPU usage 0-100%
//    uint16_t errorCount;        // Number of errors since start
//};
//
///**
// * @brief Application suspend command message
// *
// * Kernel commands application to suspend operations.
// * Application acknowledges by signaling completion.
// */
//struct AppSuspendCommandMsg : public MessageBase {
//    AppSuspendCommandMsg()
//        : MessageBase(MessageType::COMMAND_APP_SUSPEND)
//        , reasonCode(0)
//        , suspendAcknowledged(false)
//    {}
//
//    // Request fields (filled by kernel)
//    uint8_t reasonCode;         // Reason for suspend (0=low_power, 1=user_request, etc.)
//
//    // Response fields (filled by application)
//    bool suspendAcknowledged;   // True if application ready to suspend
//};


} // namespace SDK
