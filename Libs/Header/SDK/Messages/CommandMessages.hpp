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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestAppTerminate) == 36, "RequestAppTerminate size must be 36 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestSetCapabilities) == 36, "RequestSetCapabilities size must be 36 bytes");
#endif
// External-HR acquisition is opt-in via REQUEST_ACCESSORY_PREPARE / _RELEASE
// (AccessoryMessages.hpp) — a new message type, so it carries no ABI risk for
// apps built against an older capabilities layout.

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestBatteryStatus) == 44, "RequestBatteryStatus size must be 44 bytes");
#endif

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

    uint32_t heightCm;      // User height in centimeters
    float    weightKg;      // User weight in kilograms

    RequestSystemSettings()
        : MessageBase(MessageType::REQUEST_SYSTEM_SETTINGS)
        , languageId(0)
        , imperialUnits(false)
        , timeFormat(false)
        , heartRateCount(0)
        , heartRateTh {}
        , activityMin(0)
        , steps(0)
        , floors(0)
        , heightCm(0)
        , weightKg(0.0f)
    {}
};
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestSystemSettings) == 64, "RequestSystemSettings size must be 64 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestSystemInfo) == 72, "RequestSystemInfo size must be 72 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestMemoryInfo) == 52, "RequestMemoryInfo size must be 52 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestDisplayConfig) == 40, "RequestDisplayConfig size must be 40 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestDisplayUpdate) == 44, "RequestDisplayUpdate size must be 44 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestBacklightSet) == 40, "RequestBacklightSet size must be 40 bytes");
#endif

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
        uint8_t volume = 100;   // 0-100%, 0 - no sound. (Currently supported only 4 levels (0, 33, 66, 100))
    };

    uint32_t notesCount;
    Note notes[skMaxNotes];

    RequestBuzzerPlay()
        : MessageBase(MessageType::REQUEST_BUZZER_PLAY)
        , notesCount(0)
        , notes {}
    {}
};
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestBuzzerPlay) == 116, "RequestBuzzerPlay size must be 116 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestVibroPlay) == 100, "RequestVibroPlay size must be 100 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(EventSystemPowerLow) == 36, "EventSystemPowerLow size must be 36 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(EventSystemPowerCritical) == 36, "EventSystemPowerCritical size must be 36 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(EventGuiTick) == 40, "EventGuiTick size must be 40 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(EventButton) == 40, "EventButton size must be 40 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestGlanceConfig) == 40, "RequestGlanceConfig size must be 40 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestGlanceUpdate) == 44, "RequestGlanceUpdate size must be 44 bytes");
#endif

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
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(EventGlanceTick) == 40, "EventGlanceTick size must be 40 bytes");
#endif

/**
 * @brief Notify service that glance mode stopped
 */
struct EventGlanceStop : public MessageBase {
    EventGlanceStop() : MessageBase(MessageType::EVENT_GLANCE_STOP) {}
};

// -- Widget (Service -> kernel: live home-screen element) ---------------------

/** @brief Widget label buffer size in bytes (UTF-8, including the NUL). */
inline constexpr uint32_t WIDGET_TEXT_BYTES = 64;

/**
 * @brief Which fields of a widget update the kernel should show (bitmask).
 *
 * Authoritative / replace semantics: each update fully specifies what the widget
 * shows *now*. Only fields whose bit is set in `shown` are displayed; every other
 * field is hidden (a field is cleared simply by leaving its bit unset).
 *
 * A bit either gates a field (WIDGET_SHOW_TEXT / WIDGET_SHOW_PERCENT) or modifies
 * how a shown field renders (WIDGET_PROGRESS_FROM_END).
 *
 * Cross-version compatible by design. New field kinds (e.g. an icon) are added by
 * APPENDING a field to RequestWidgetUpdate and a new bit here -- never reorder,
 * remove, or reuse existing fields/bits, so the offsets of the old fields stay
 * put. An older sender simply never sets the newer bits, and an older reader
 * ignores bits it does not know (an unknown field stays hidden; an unknown
 * modifier is simply not applied, so the field renders its default way).
 *
 * The message is a plain struct over IPC -- it cannot validate what the sender
 * writes -- so the guarantee lives on the READER (the kernel): mask `shown`
 * against the bits it knows (`shown & WIDGET_SHOW_ALL`) and read a field only
 * when its known bit is set. That drops any unknown or garbage bit. And because
 * every field is zero-initialised (see the ctor), even a stray known bit reads a
 * valid default (empty label, 0 percent) -- never uninitialised memory. So the
 * worst a bad `shown` can do is blank a field, never crash or over-read.
 */
enum WidgetShow : uint32_t {
    WIDGET_SHOW_TEXT    = 1u << 0,   ///< Display `text`.
    WIDGET_SHOW_PERCENT = 1u << 1,   ///< Display `percent` as a progress bar.

    /**
     * @brief Anchor the progress fill at the bar's end (default: its start).
     *
     * Modifier for WIDGET_SHOW_PERCENT (meaningless without it). The bar colours
     * `percent` of the track; this bit fixes the point that share is measured
     * from. Start and end are logical positions -- the kernel maps them to the
     * widget's actual layout and orientation.
     *
     * Clear: the fill grows from the start (the 0% end); a rising percent fills
     * the bar -- e.g. steps toward a goal. Set: the fill is pinned to the end
     * (the 100% end); a falling percent recedes to that end and empties the bar
     * from the start -- e.g. a countdown timer sending its remaining share.
     */
    WIDGET_PROGRESS_FROM_END = 1u << 2,

    /** @brief All bits this SDK version defines -- readers mask `shown` with this. */
    WIDGET_SHOW_ALL     = WIDGET_SHOW_TEXT | WIDGET_SHOW_PERCENT | WIDGET_PROGRESS_FROM_END,
};

/**
 * @brief Widget start request.
 *
 * A running app claims the single home-screen widget slot for an ongoing
 * activity; the kernel begins showing an element for it. The app is identified by
 * the sender's process (its name and icon come from the .uapp), so no payload is
 * needed. Follow with RequestWidgetUpdate to fill in content, RequestWidgetStop
 * to end it.
 *
 * Ownership is last-start-wins: a START from another app takes the slot over, and
 * from then on only that new owner's UPDATE/STOP take effect. There is no owner
 * stack -- when the owner STOPs, the slot is cleared, not handed back to a
 * previously displaced app (which must START again to show). The kernel also
 * clears the slot if the owning process dies.
 */
struct RequestWidgetStart : public MessageBase {
    RequestWidgetStart() : MessageBase(MessageType::REQUEST_WIDGET_START) {}
};

/**
 * @brief Widget update request.
 *
 * Replaces the home-screen element's content: `shown` (a WidgetShow bitmask) is
 * the authoritative set of fields to display right now -- label, progress bar, or
 * both -- and fields left out are hidden. Sent whenever the app chooses; there is
 * no fixed cadence. Fire-and-forget.
 *
 * Takes effect only from the slot's current owner: an update before the owner's
 * START, after its STOP, or from a displaced app is ignored.
 */
struct RequestWidgetUpdate : public MessageBase {
    uint32_t shown;                    // Bitmask of WidgetShow: fields to display now
    float    percent;                  // Share of the bar to colour, 0..100 (if
                                       // WIDGET_SHOW_PERCENT), measured from the bar's
                                       // start or its end (WIDGET_PROGRESS_FROM_END); a
                                       // reader clamps out-of-range / NaN to that range
    char     text[WIDGET_TEXT_BYTES];  // Label (if WIDGET_SHOW_TEXT); UTF-8, NUL-terminated

    // shown MUST default to 0 -- allocateMessage() placement-news every message,
    // so this guarantees a clean bitmask (no uninitialised bits): an unset field
    // is never read. That is what makes the cross-version contract above safe;
    // do not drop this zero-init, and only ever assign shown (never |= it onto an
    // unconstructed value).
    RequestWidgetUpdate()
        : MessageBase(MessageType::REQUEST_WIDGET_UPDATE)
        , shown(0)
        , percent(0.0f)
        , text{}
    {}
};
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RequestWidgetUpdate) == 104, "RequestWidgetUpdate size must be 104 bytes");
#endif

/**
 * @brief Widget stop request.
 *
 * The activity ended; the kernel clears the slot. Effective only from the current
 * owner, and it does not restore any previously displaced app (no owner stack).
 */
struct RequestWidgetStop : public MessageBase {
    RequestWidgetStop() : MessageBase(MessageType::REQUEST_WIDGET_STOP) {}
};

} // namespace SDK::Message

#pragma pack(pop)
