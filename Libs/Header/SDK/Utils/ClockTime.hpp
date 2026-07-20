/**
 ******************************************************************************
 * @file    ClockTime.hpp
 * @brief   Shared 12/24-hour time-of-day conversion.
 *
 * Semantics only: the SDK carries the 12/24 choice; how a face displays it
 * (padding, separator, AM/PM glyphs, layout) is a per-face design choice.
 ******************************************************************************
 */
#ifndef SDK_UTILS_CLOCKTIME_HPP
#define SDK_UTILS_CLOCKTIME_HPP

#include <cstdint>

namespace SDK::Clock
{

/// A 24-hour reading expressed on a 12-hour clock.
struct Hour12
{
    std::uint8_t hour;  ///< 1..12
    bool         pm;    ///< false = AM, true = PM
};

/**
 * @brief Convert a 0-23 hour to a 12-hour clock reading.
 *
 * Precondition: @p hour24 is in [0, 23]. Boundary mapping:
 *   0 -> 12 AM,  1..11 -> 1..11 AM,  12 -> 12 PM,  13..23 -> 1..11 PM.
 */
constexpr Hour12 to12Hour(std::uint8_t hour24) noexcept
{
    const bool         pm  = hour24 >= 12u;
    const std::uint8_t rem = static_cast<std::uint8_t>(hour24 % 12u);
    return Hour12{ rem == 0u ? static_cast<std::uint8_t>(12) : rem, pm };
}

/**
 * @brief Inverse of to12Hour(): a 12-hour reading + AM/PM back to a 0-23 hour.
 *
 * Precondition: @p hour12 is in [1, 12]. (12, AM) -> 0, (12, PM) -> 12.
 */
constexpr std::uint8_t to24Hour(std::uint8_t hour12, bool pm) noexcept
{
    const std::uint8_t base = static_cast<std::uint8_t>(hour12 % 12u);  // 12 -> 0
    return static_cast<std::uint8_t>(base + (pm ? 12u : 0u));
}

} // namespace SDK::Clock

#endif // SDK_UTILS_CLOCKTIME_HPP
