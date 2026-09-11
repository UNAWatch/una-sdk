/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages exchanged between the Console clockface service and GUI.
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
constexpr SDK::MessageType::Type TIME         = 0x00000003;
constexpr SDK::MessageType::Type STEPS        = 0x00000004;
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
 * @brief The day's step count, the one reading this face shows.
 *
 * The face draws no charge level, no active minutes and no heart rate, so the
 * service subscribes to none of them. That is not only tidiness: a heart-rate
 * subscription keeps the optical sensor running, which is the most expensive
 * thing a clockface could ask for and would be paid for whenever this face is
 * the one on screen.
 */
struct Steps : public SDK::MessageBase {
    uint32_t steps;     ///< Step count for the current day

    Steps()
        : SDK::MessageBase(STEPS)
        , steps(0)
    {}

    explicit Steps(uint32_t steps)
        : Steps()
    {
        this->steps = steps;
    }
};

/**
 * @brief How the watch is set to write the time and the date.
 *
 * Both are read from the kernel's system settings, which are pull-only: there
 * is no event when either changes, so the service re-reads them when the GUI
 * starts, whenever the GUI asks with @ref Refresh, and on a slow poll.
 *
 * The date order reaches this face differently from the other three. They
 * write a line and reverse two of its parts; this one stacks the date, so the
 * order swaps the two outer rows and leaves the day of the month alone.
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
 * format changes.
 */
struct Refresh : public SDK::MessageBase {
    Refresh()
        : SDK::MessageBase(REFRESH)
    {}
};

// The kernel's message pools top out at 256 bytes and an oversized send is
// dropped silently; the simulator allocates with new[] and would not show it.
static_assert(sizeof(Time) <= 256, "Time must fit the largest kernel message pool");
static_assert(sizeof(Steps) <= 256, "Steps must fit the largest kernel message pool");
static_assert(sizeof(ClockFormat) <= 256, "ClockFormat must fit the largest kernel message pool");
static_assert(sizeof(Refresh) <= 256, "Refresh must fit the largest kernel message pool");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
