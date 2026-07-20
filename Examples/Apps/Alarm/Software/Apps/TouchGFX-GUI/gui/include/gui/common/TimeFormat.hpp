#ifndef TIMEFORMAT_HPP
#define TIMEFORMAT_HPP

#include <cstdint>

/**
 * @brief Helpers for converting between the stored 24-hour time and the
 *        12-hour clock shown when the system clock-format setting is 12H.
 *
 * Alarms are always stored as a 0-23 hour; only the presentation and the
 * edit wheels switch to 12-hour.
 */
namespace App::TimeFormat {

/** @brief Split a 0-23 hour into a 1-12 hour and an AM/PM flag. */
inline void split12(uint8_t hour24, uint8_t& hour12, bool& pm)
{
    pm     = (hour24 >= 12);
    hour12 = static_cast<uint8_t>(hour24 % 12);
    if (hour12 == 0) {
        hour12 = 12;
    }
}

/** @brief Combine a 1-12 hour and an AM/PM flag back into a 0-23 hour. */
inline uint8_t to24(uint8_t hour12, bool pm)
{
    const uint8_t base = static_cast<uint8_t>(hour12 % 12);   // 12 -> 0
    return static_cast<uint8_t>(base + (pm ? 12 : 0));
}

} // namespace App::TimeFormat

#endif // TIMEFORMAT_HPP
