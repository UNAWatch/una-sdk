/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages exchanged between the Smile clockface service and GUI.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type BATTERY      = 0x00000001;
constexpr SDK::MessageType::Type ALERTS_MUTED = 0x00000002;
constexpr SDK::MessageType::Type TIME         = 0x00000003;
constexpr SDK::MessageType::Type HEALTH       = 0x00000004;
constexpr SDK::MessageType::Type CLOCK_FORMAT = 0x00000005;

// GUI --> Service
constexpr SDK::MessageType::Type REFRESH      = 0x00000010;

/**
 * @brief The local time, to the minute, and the date that goes with it.
 *
 * The face draws nothing finer than a minute, so nothing finer is carried.
 * Sent when the minute turns.
 */
struct Time : public SDK::MessageBase {
    uint8_t hour;       ///< 0..23, always; the GUI is what folds it to 12
    uint8_t minute;     ///< 0..59
    uint8_t mday;       ///< Day of month, 1..31
    uint8_t wday;       ///< Day of week, 0 = Sunday, as std::tm::tm_wday
    uint8_t mon;        ///< Month, 0 = January, as std::tm::tm_mon

    Time()
        : SDK::MessageBase(TIME)
        , hour(0)
        , minute(0)
        , mday(0)
        , wday(0)
        , mon(0)
    {}

    Time(uint8_t hour, uint8_t minute, uint8_t mday, uint8_t wday, uint8_t mon)
        : Time()
    {
        this->hour   = hour;
        this->minute = minute;
        this->mday   = mday;
        this->wday   = wday;
        this->mon    = mon;
    }
};

/**
 * @brief The charge level the service last read from the sensor layer.
 *
 * The sensor is event driven, so one of these arrives when the connection
 * opens and then only when the level actually moves.
 */
struct Battery : public SDK::MessageBase {
    uint8_t level;      ///< Charge, percent (0-100)

    Battery()
        : SDK::MessageBase(BATTERY)
        , level(0)
    {}

    explicit Battery(uint8_t level)
        : Battery()
    {
        this->level = level;
    }
};

/**
 * @brief Whether the watch is currently silencing its alerts.
 *
 * Carried for the same reason the Analogue face carries it: the path exists
 * and is exercised, but the SDK exposes no mute state for an app to read, so
 * nothing calls the publisher. This face draws no mute icon either way.
 */
struct AlertsMuted : public SDK::MessageBase {
    bool muted;

    AlertsMuted()
        : SDK::MessageBase(ALERTS_MUTED)
        , muted(false)
    {}

    explicit AlertsMuted(bool muted)
        : AlertsMuted()
    {
        this->muted = muted;
    }
};

/**
 * @brief The three daily readings the face shows below the clock.
 *
 * All three travel together because they change on the same order of
 * timescale and the face redraws the block as one; splitting them would cost
 * three IPC round trips to say the same thing.
 *
 * @c bpm is zero when there is no reading worth showing -- the service has
 * already applied the trust gate, so the GUI only has to decide what to draw
 * in its place.
 */
struct Health : public SDK::MessageBase {
    uint32_t steps;             ///< Step count for the current day
    uint32_t activityMinutes;   ///< Active minutes for the current day
    uint16_t bpm;               ///< Heart rate, or 0 for "no trusted reading"

    Health()
        : SDK::MessageBase(HEALTH)
        , steps(0)
        , activityMinutes(0)
        , bpm(0)
    {}

    Health(uint32_t steps, uint32_t activityMinutes, uint16_t bpm)
        : Health()
    {
        this->steps           = steps;
        this->activityMinutes = activityMinutes;
        this->bpm             = bpm;
    }
};

/**
 * @brief How the watch is set to write the time and the date.
 *
 * Both are read from the kernel's system settings, which are pull-only: there
 * is no event when either changes, so the service re-reads them when the GUI
 * starts, whenever the GUI asks with @ref Refresh, and on a slow poll.
 *
 * They travel together because they are one decision to the face: each arrives
 * from the same read and the date line carries both of them -- the day and
 * month in the chosen order, and the meridiem when the clock is 12-hour.
 */
struct ClockFormat : public SDK::MessageBase {
    bool is12h;
    bool monthFirst;

    ClockFormat()
        : SDK::MessageBase(CLOCK_FORMAT)
        , is12h(false)
        , monthFirst(false)
    {}

    ClockFormat(bool is12h, bool monthFirst)
        : ClockFormat()
    {
        this->is12h      = is12h;
        this->monthFirst = monthFirst;
    }
};

/**
 * @brief GUI to service: everything you know, again, now.
 *
 * Sent when the GUI resumes. The kernel suspends a clockface rather than
 * stopping it, so coming back from Settings brings no lifecycle message the
 * service could hang a re-read off -- and Settings is exactly where the clock
 * format changes. Answering this republishes the format, so a face that was
 * off screen while the setting changed is right on its first frame back.
 */
struct Refresh : public SDK::MessageBase {
    Refresh()
        : SDK::MessageBase(REFRESH)
    {}
};

// The kernel's message pools top out at 256 bytes and an oversized send is
// dropped silently; the simulator allocates with new[] and would not show it.
static_assert(sizeof(Time) <= 256, "Time must fit the largest kernel message pool");
static_assert(sizeof(Battery) <= 256, "Battery must fit the largest kernel message pool");
static_assert(sizeof(AlertsMuted) <= 256, "AlertsMuted must fit the largest kernel message pool");
static_assert(sizeof(Health) <= 256, "Health must fit the largest kernel message pool");
static_assert(sizeof(ClockFormat) <= 256, "ClockFormat must fit the largest kernel message pool");
static_assert(sizeof(Refresh) <= 256, "Refresh must fit the largest kernel message pool");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
