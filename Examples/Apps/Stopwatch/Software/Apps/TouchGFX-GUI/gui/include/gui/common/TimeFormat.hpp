/**
 ******************************************************************************
 * @file    TimeFormat.hpp
 * @date    17-07-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Millisecond duration formatting for the stopwatch display.
 ******************************************************************************
 *
 * SDK::Utils::toHMS works in whole seconds, so it cannot express the
 * hundredths this screen shows. These helpers fill that gap.
 *
 ******************************************************************************
 */

#ifndef TIMEFORMAT_HPP
#define TIMEFORMAT_HPP

#include <cstdint>

#include <touchgfx/Unicode.hpp>

namespace TimeFormat
{

/**
 * @brief The display is capped at 99:59:59.99; a longer run freezes there.
 */
constexpr uint32_t kMaxMs = (99u * 3600u + 59u * 60u + 59u) * 1000u + 990u;

/**
 * @brief A duration broken into display fields.
 */
struct Parts
{
    uint8_t h;   ///< Hours, 0-99
    uint8_t m;   ///< Minutes, 0-59
    uint8_t s;   ///< Seconds, 0-59
    uint8_t cs;  ///< Hundredths, 0-99
};

/**
 * @brief Split a duration into display fields, clamped to 99:59:59.99.
 * @param ms Duration in milliseconds.
 */
inline Parts split(uint32_t ms)
{
    if (ms > kMaxMs) {
        ms = kMaxMs;
    }
    Parts p;
    p.cs = static_cast<uint8_t>((ms / 10) % 100);
    const uint32_t totalSec = ms / 1000;
    p.s = static_cast<uint8_t>(totalSec % 60);
    p.m = static_cast<uint8_t>((totalSec / 60) % 60);
    p.h = static_cast<uint8_t>(totalSec / 3600);
    return p;
}

/**
 * @brief True once the duration reaches an hour, where the reading widens to
 *        HH:MM:SS and the hundredths are dropped.
 */
inline bool hasHours(uint32_t ms)
{
    return ms >= 3600u * 1000u;
}

/**
 * @brief Write the main field: "MM:SS" under an hour, "HH:MM:SS" from an hour.
 * @param ms   Duration in milliseconds.
 * @param buf  Destination buffer.
 * @param size Capacity of buf in characters.
 */
inline void mainField(uint32_t ms, touchgfx::Unicode::UnicodeChar *buf, uint16_t size)
{
    const Parts p = split(ms);
    if (p.h > 0) {
        touchgfx::Unicode::snprintf(buf, size, "%02u:%02u:%02u", p.h, p.m, p.s);
    } else {
        touchgfx::Unicode::snprintf(buf, size, "%02u:%02u", p.m, p.s);
    }
}

/**
 * @brief Write the hundredths field: "CC".
 *
 * The GUI is ticked at 10 Hz, so this field is accurate when drawn but only
 * refreshes ten times a second rather than stepping through every value.
 *
 * @param ms   Duration in milliseconds.
 * @param buf  Destination buffer.
 * @param size Capacity of buf in characters.
 */
inline void fracField(uint32_t ms, touchgfx::Unicode::UnicodeChar *buf, uint16_t size)
{
    touchgfx::Unicode::snprintf(buf, size, "%02u", split(ms).cs);
}

/**
 * @brief Write a lap duration: "MM:SS.CC", widening to "HH:MM:SS" from an hour.
 * @param ms   Duration in milliseconds.
 * @param buf  Destination buffer.
 * @param size Capacity of buf in characters.
 */
inline void lapField(uint32_t ms, touchgfx::Unicode::UnicodeChar *buf, uint16_t size)
{
    const Parts p = split(ms);
    if (p.h > 0) {
        touchgfx::Unicode::snprintf(buf, size, "%02u:%02u:%02u", p.h, p.m, p.s);
    } else {
        touchgfx::Unicode::snprintf(buf, size, "%02u:%02u.%02u", p.m, p.s, p.cs);
    }
}

} // namespace TimeFormat

#endif // TIMEFORMAT_HPP
