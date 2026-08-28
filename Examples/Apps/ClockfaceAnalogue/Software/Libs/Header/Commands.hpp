/**
 ******************************************************************************
 * @file    Commands.hpp
 * @date    27-August-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Messages exchanged between the Analogue clockface service and GUI.
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

/**
 * @brief The local time, to the minute, and the date that goes with it.
 *
 * The face draws nothing finer than a minute, so nothing finer is carried.
 * Sent when the minute turns.
 */
struct Time : public SDK::MessageBase {
    uint8_t hour;       ///< 0..23
    uint8_t minute;     ///< 0..59
    uint8_t mday;       ///< Day of month, 1..31
    uint8_t wday;       ///< Day of week, 0 = Sunday, as std::tm::tm_wday

    Time()
        : SDK::MessageBase(TIME)
        , hour(0)
        , minute(0)
        , mday(0)
        , wday(0)
    {}

    Time(uint8_t hour, uint8_t minute, uint8_t mday, uint8_t wday)
        : Time()
    {
        this->hour   = hour;
        this->minute = minute;
        this->mday   = mday;
        this->wday   = wday;
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
 * The face shows a struck-through speaker while this is set. Sent on the same
 * terms as the charge level: once when the state is first known, and then only
 * when it changes.
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

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
