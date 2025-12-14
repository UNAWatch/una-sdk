/**
 * @file    CommandMessages.hpp
 * @date    11-12-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Command message definitions (kernel <-> application)
 *
 * All message structures use 4-byte alignment for serialization compatibility.
 * Field order is fixed and must not be changed to maintain binary compatibility.
 */

#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Glance/GlanceControl.h"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace SDK::Message
{

// =============================================================================
// Simple command messages (no additional data)
// =============================================================================

/**
 * @brief Application run command
 * 
 * Sent to Service and GUI as first message after application start.
 * Apps should initialize resources in response to this command.
 */
struct CommandAppRun : public MessageBase {
    CommandAppRun() : MessageBase(MessageType::COMMAND_APP_RUN) {}
};

/**
 * @brief Application stop command
 * 
 * Sent to Service and GUI as last message before application destruction.
 * Apps should cleanup resources in response to this command.
 */
struct CommandAppStop : public MessageBase {
    CommandAppStop() : MessageBase(MessageType::COMMAND_APP_STOP) {}
};

/**
 * @brief GUI resume command
 * 
 * Sent to GUI when display turns on or app returns to foreground.
 * GUI should resume rendering.
 */
struct CommandAppGuiResume : public MessageBase {
    CommandAppGuiResume() : MessageBase(MessageType::COMMAND_APP_GUI_RESUME) {}
};

/**
 * @brief GUI suspend command
 * 
 * Sent to GUI when display turns off or app goes to background.
 * GUI should pause rendering and animations.
 */
struct CommandAppGuiSuspend : public MessageBase {
    CommandAppGuiSuspend() : MessageBase(MessageType::COMMAND_APP_GUI_SUSPEND) {}
};

/**
 * @brief Notify Service that GUI has started
 *
 * Sent by kernel to Service when GUI part starts running.
 * Service can now communicate with GUI if needed.
 */
struct CommandAppNotifGuiRun : public MessageBase {
    CommandAppNotifGuiRun() : MessageBase(MessageType::COMMAND_APP_NOTIF_GUI_RUN) {}
};

/**
 * @brief Notify Service that GUI has stopped
 *
 * Sent by kernel to Service when GUI part stops.
 * Service should stop sending messages to GUI.
 */
struct CommandAppNotifGuiStop : public MessageBase {
    CommandAppNotifGuiStop() : MessageBase(MessageType::COMMAND_APP_NOTIF_GUI_STOP){}
};

// =============================================================================
// Request messages with data
// =============================================================================

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
 * auto* req = srvComm->allocateMessage<RequestAppRunGui>();
 * if (srvComm->sendMessage(req)) {
 *     if (req->getResult() == MessageResult::SUCCESS) {
 *         // GUI is now running, can send internal messages
 *     }
 * }
 * srvComm->releaseMessage(req);
 * @endcode
 */
struct RequestAppRunGui : public MessageBase {
    RequestAppRunGui() : MessageBase(MessageType::REQUEST_APP_RUN_GUI) {}
};

/**
 * @brief The application requests to be terminated.
 *
 * This request should be executed when the application has freed
 * all resources and is no longer needed. Or when an unrecoverable
 * error has occurred (hardfault, assert, etc.)
 */
struct RequestAppTerminate : public MessageBase {
    int32_t code;  // Exit code (0 = normal, non-zero = error)

    RequestAppTerminate()
        : MessageBase(MessageType::REQUEST_APP_TERMINATE)
        , code(0)
    {}
};
static_assert(sizeof(RequestAppTerminate) == 36, "RequestAppTerminate size must be 36 bytes");

/**
 * @brief Notify the Kernel that the activity session has ended.
 *
 * Sent by Service/GUI to to Kernel after saving the activity file.
 */
struct CommandAppNewActivity : public MessageBase {
    CommandAppNewActivity() : MessageBase(MessageType::REQUEST_APP_NEW_ACTIVITY){}
};

/**
 * @brief Set application capabilities
 *
 * Service sends this to configure app behavior.
 */
struct RequestSetCapabilities : public MessageBase {
    bool enPhoneNotification;  // Enable phone notifications during app
    bool enUsbChargingScreen;  // Enable USB charging screen during app
    bool enMusicControl;       // Enable music control during app

    RequestSetCapabilities()
        : MessageBase(MessageType::REQUEST_SET_CAPABILITIES)
        , enPhoneNotification(false)
        , enUsbChargingScreen(false)
        , enMusicControl(false)
    {}
};
static_assert(sizeof(RequestSetCapabilities) == 36, "RequestSetCapabilities size must be 36 bytes");

/**
 * @brief Battery status request
 *
 * App requests current battery information.
 */
struct RequestBatteryStatus : public MessageBase {
    // Response fields
    float batteryLevel; // 0-100%
    float voltage;      // Volts
    bool  isCharging;

    RequestBatteryStatus()
        : MessageBase(MessageType::REQUEST_BATTERY_STATUS)
        , batteryLevel(0.0f)
        , voltage(0.0f)
        , isCharging(false)
    {}
};
static_assert(sizeof(RequestBatteryStatus) == 44, "RequestBatteryStatus size must be 44 bytes");

/**
 * @brief System settings request
 *
 * App requests system configuration.
 */
struct RequestSystemSettings : public MessageBase {

    // Maximum HR thresholds (4 thresholds = 5 zones)
    static const uint32_t skMaxHearRateTh = 8;

    // Response fields
    uint8_t languageId;      // System language
    bool    imperialUnits;   // Units imperial/metric
    bool    timeFormat;      // 12/24 hour format

    uint8_t heartRateCount;
    uint8_t heartRateTh[skMaxHearRateTh];

    uint32_t activityMin;   // target number of active minutes per day.
    uint32_t steps;         // target number of steps per day
    uint32_t floors;        // target number of floors climbed per day

    RequestSystemSettings()
        : MessageBase(MessageType::REQUEST_SYSTEM_SETTINGS)
        , languageId(0)
        , imperialUnits(false)
        , timeFormat(false)
        , heartRateCount(0)
        , heartRateTh {}
    {}
};
static_assert(sizeof(RequestSystemSettings) == 56, "RequestSystemSettings size must be 56 bytes");

/**
 * @brief System information request
 *
 * App requests system information (firmware, hardware, etc.)
 */
struct RequestSystemInfo : public MessageBase {
    // Response fields
    char firmwareVersion[16];  // e.g. "1.0.5"
    char hardwareVersion[16];  // e.g. "HW_v2.1"
    uint32_t uptimeSeconds;    // System uptime
    uint8_t cpuUsagePercent;   // Current CPU usage 0-100%

    RequestSystemInfo()
        : MessageBase(MessageType::REQUEST_SYSTEM_INFO)
        , uptimeSeconds(0)
        , cpuUsagePercent(0)
    {
        firmwareVersion[0] = '\0';
        hardwareVersion[0] = '\0';
    }
};
static_assert(sizeof(RequestSystemInfo) == 72, "RequestSystemInfo size must be 72 bytes");

/**
 * @brief Memory information request
 *
 * App requests memory statistics.
 */
struct RequestMemoryInfo : public MessageBase {
    // Response fields
    uint32_t totalHeap;        // Total heap size (bytes)
    uint32_t freeHeap;         // Free heap (bytes)
    uint32_t usedHeap;         // Used heap (bytes)
    uint32_t largestFreeBlock; // Largest contiguous free block (bytes)
    uint16_t fragmentation;    // Fragmentation percentage 0-100%

    RequestMemoryInfo()
        : MessageBase(MessageType::REQUEST_MEMORY_INFO)
        , totalHeap(0)
        , freeHeap(0)
        , usedHeap(0)
        , largestFreeBlock(0)
        , fragmentation(0)
    {}
};
static_assert(sizeof(RequestMemoryInfo) == 52, "RequestMemoryInfo size must be 52 bytes");

/**
 * @brief Display configuration request
 *
 * GUI requests display parameters.
 */
struct RequestDisplayConfig : public MessageBase {
    // Response fields
    int16_t width;         // Display width in pixels
    int16_t height;        // Display height in pixels
    uint8_t colorDepth;    // Bits per pixel

    // Reserved. Not used. Format ABGR2222
    uint8_t format;        // Pixel format (RGB565, RGB888, etc.)

    RequestDisplayConfig()
        : MessageBase(MessageType::REQUEST_DISPLAY_CONFIG)
        , width(0)
        , height(0)
        , colorDepth(0)
        , format(0)
    {}
};
static_assert(sizeof(RequestDisplayConfig) == 40, "RequestDisplayConfig size must be 40 bytes");

/**
 * @brief Display update request
 *
 * GUI sends frame buffer to be displayed.
 * @note Buffer must remain valid until response received.
 * @note The buffer size in bytes should be:
 *       width * height * (color depth + 7) / 8.
 */
struct RequestDisplayUpdate : public MessageBase {
    const uint8_t* pBuffer;  // Pointer to frame buffer

    // Reserved. Not used. Always update the entire buffer.
    int16_t x, y;            // Update region top-left
    int16_t width, height;   // Update region size (0 = full screen)

    RequestDisplayUpdate()
        : MessageBase(MessageType::REQUEST_DISPLAY_UPDATE)
        , pBuffer(nullptr)
        , x(0), y(0)
        , width(0), height(0)
    {}
};
static_assert(sizeof(RequestDisplayUpdate) == 44, "RequestDisplayUpdate size must be 44 bytes");

/**
 * @brief Backlight control request
 *
 * Set backlight brightness and auto-off timeout.
 */
struct RequestBacklightSet : public MessageBase {
    uint8_t brightness;         // 0-100%, 0 = off
    uint32_t autoOffTimeoutMs;  // Auto-off timeout, 0 = disabled

    RequestBacklightSet()
        : MessageBase(MessageType::REQUEST_BACKLIGHT_SET)
        , brightness(100)
        , autoOffTimeoutMs(0)
    {}
};
static_assert(sizeof(RequestBacklightSet) == 40, "RequestBacklightSet size must be 40 bytes");

/**
 * @brief Buzzer play request
 *
 * Play buzzer pattern.
 */
struct RequestBuzzerPlay : public MessageBase {
    // Maximum Notes includes pauses.
    static const uint32_t skMaxNotes = 10;

    struct Note {
        uint32_t time = 100;    // Duration in ms
        uint8_t volume;         // 0-100%, 0 - no sound. (Currently supported only 4 levels (0, 33, 66, 100))
    };

    uint32_t notesCount;
    Note notes[skMaxNotes];

    RequestBuzzerPlay()
        : MessageBase(MessageType::REQUEST_BUZZER_PLAY)
        , notesCount(0)
        , notes {}
    {}
};
static_assert(sizeof(RequestBuzzerPlay) == 116, "RequestBuzzerPlay size must be 116 bytes");

/**
 * @brief Vibration play request
 */
struct RequestVibroPlay : public MessageBase {
    // Predefined effects
    enum Effect {
        NO_EFFECT                            = 0,  // silent
        STRONG_CLICK_100                     = 1,
        SHARP_CLICK_100                      = 4,
        SOFT_BUMP_100                        = 7,
        DOUBLE_CLICK_100                     = 10,
        STRONG_BUZZ_100                      = 14,
        ALERT_750MS_100                      = 15,
        ALERT_1000MS_100                     = 16,
        STRONG_CLICK_1_100                   = 17,
        MEDIUM_CLICK_1_100                   = 21,
        SHARP_TICK_1_100                     = 24,
        SHORT_DOUBLE_CLICK_STRONG_1_100      = 27,
        SHORT_DOUBLE_CLICK_MEDIUM_1_100      = 31,
        SHORT_DOUBLE_SHARP_TICK_1_100        = 34,
        LONG_DOUBLE_SHARP_CLICK_STRONG_1_100 = 37,
        LONG_DOUBLE_SHARP_CLICK_MEDIUM_1_100 = 41,
        LONG_DOUBLE_SHARP_TICK_1_100         = 44,
        BUZZ_1_100                           = 47,
        PULSING_STRONG_1_100                 = 52,
        PULSING_MEDIUM_1_100                 = 54,
        PULSING_SHARP_1_100                  = 56,
    };

    // Maximum Notes includes pauses.
    static const uint32_t skMaxNotes = 8;

    struct Note {
        uint8_t  effect;    // 1 - 127,  0 - for pause
        uint32_t pause;     // Pause duration in ms. 0 - if effect specified.
    };

    uint32_t notesCount;
    Note notes[skMaxNotes];

    RequestVibroPlay()
        : MessageBase(MessageType::REQUEST_VIBRO_PLAY)
        , notesCount(0)
        , notes {}
    {}
};
static_assert(sizeof(RequestVibroPlay) == 100, "RequestVibroPlay size must be 100 bytes");

// =============================================================================
// Event messages
// =============================================================================

/**
 * @brief System power low event
 *
 * Sent when battery reaches low level (e.g. 20%).
 * App should reduce power consumption.
 */
struct EventSystemPowerLow : public MessageBase {
    uint8_t batteryLevel;       // Current battery level 0-100%
    uint16_t estimatedMinutes;  // Estimated minutes remaining

    EventSystemPowerLow()
        : MessageBase(MessageType::EVENT_SYSTEM_POWER_LOW)
        , batteryLevel(0)
        , estimatedMinutes(0)
    {}
};
static_assert(sizeof(EventSystemPowerLow) == 36, "EventSystemPowerLow size must be 36 bytes");

/**
 * @brief System power critical event
 *
 * Sent when battery reaches critical level (e.g. 5%).
 * App should save state and prepare for shutdown.
 */
struct EventSystemPowerCritical : public MessageBase {
    uint8_t batteryLevel;       // Current battery level 0-100%
    uint16_t estimatedMinutes;  // Estimated minutes remaining (usually <5)

    EventSystemPowerCritical()
        : MessageBase(MessageType::EVENT_SYSTEM_POWER_CRITICAL)
        , batteryLevel(0)
        , estimatedMinutes(0)
    {}
};
static_assert(sizeof(EventSystemPowerCritical) == 36, "EventSystemPowerCritical size must be 36 bytes");

/**
 * @brief GUI frame tick command
 *
 * Sent to GUI periodically (e.g. 60 Hz) for animation timing.
 * GUI can use this for smooth animations.
 */
struct EventGuiTick : public MessageBase {
    uint32_t frameNumber;  // Incremental frame counter
    uint32_t timestamp;    // System timestamp in ms

    EventGuiTick()
        : MessageBase(MessageType::EVENT_GUI_TICK)
        , frameNumber(0)
        , timestamp(0)
    {}
};
static_assert(sizeof(EventGuiTick) == 40, "EventGuiTick size must be 40 bytes");

/**
 * @brief Button event
 *
 * Sent to GUI when button state changes.
 */
struct EventButton : public MessageBase {

    /**
     * @brief Available buttons.
     *
     * UP      Top Left        L1      SW1
     * SELECT  Top Right       R1      SW2 (PWR_ON_1V8_L)
     * DOWN    Bottom Left     L2      SW3
     * BACK    Bottom Right    R2      SW4
     */
    enum class Id : uint8_t {
        SW1 = 0,    // L1
        SW2,        // R1
        SW3,        // L2
        SW4,        // R2
    };

    /**
     * @brief Available event codes.
     */
    enum class Event : uint8_t {
       PRESS = 0,
       RELEASE,
       CLICK,
       LONG_PRESS,
       HOLD_1S,
       HOLD_5S,
       HOLD_10S,
    };

    uint32_t timestamp;
    Id id;
    Event event;

    EventButton()
        : MessageBase(MessageType::EVENT_BUTTON)
        , timestamp(0)
        , id(Id::SW1)
        , event(Event::PRESS)
    {}
};
static_assert(sizeof(EventButton) == 40, "EventButton size must be 40 bytes");

// =============================================================================
// Glance messages
// =============================================================================

/**
 * @brief Glance configuration request
 *
 * GUI requests glance area parameters.
 */
struct RequestGlanceConfig : public MessageBase {
    // Response fields
    int16_t width;
    int16_t height;
    uint32_t maxControls;

    RequestGlanceConfig()
        : MessageBase(MessageType::REQUEST_GLANCE_CONFIG)
        , width(0)
        , height(0)
        , maxControls(0)
    {}
};
static_assert(sizeof(RequestGlanceConfig) == 40, "RequestGlanceConfig size must be 40 bytes");

/**
 * @brief Glance update request
 *
 * Service updates glance content.
 * @note Data must remain valid until response received.
 */
struct RequestGlanceUpdate : public MessageBase {
    const char *name;           // Pointer to glance name
    GlanceControl_t *controls;  // Pointer to the array with glance controls
    uint32_t controlsNumber;    // Number of the controls in the array

    RequestGlanceUpdate()
        : MessageBase(MessageType::REQUEST_GLANCE_UPDATE)
        , name(nullptr)
        , controls(nullptr)
        , controlsNumber(0)
    {}
};
static_assert(sizeof(RequestGlanceUpdate) == 44, "RequestGlanceUpdate size must be 44 bytes");

/**
 * @brief Notify service that glance mode started
 */
struct EventGlanceStart : public MessageBase {
    EventGlanceStart() : MessageBase(MessageType::EVENT_GLANCE_START) {}
};

/**
 * @brief Glance frame tick command
 *
 * Sent to Service periodically (e.g. 60 Hz) for update glance content.
 */
struct EventGlanceTick : public MessageBase {
    uint32_t frameNumber;  // Incremental frame counter
    uint32_t timestamp;    // System timestamp in ms

    EventGlanceTick()
        : MessageBase(MessageType::EVENT_GLANCE_TICK)
        , frameNumber(0)
        , timestamp(0)
    {}
};
static_assert(sizeof(EventGlanceTick) == 40, "EventGlanceTick size must be 40 bytes");

/**
 * @brief Notify service that glance mode stopped
 */
struct EventGlanceStop : public MessageBase {
    EventGlanceStop() : MessageBase(MessageType::EVENT_GLANCE_STOP) {}
};

} // namespace SDK::Message

#pragma pack(pop)
