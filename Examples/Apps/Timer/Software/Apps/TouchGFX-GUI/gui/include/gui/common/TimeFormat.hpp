#ifndef TIMEFORMAT_HPP
#define TIMEFORMAT_HPP

#include <cstdint>

#include "SDK/Utils/ClockTime.hpp"

/**
 * @brief Helpers for converting between the stored 24-hour time and the
 *        12-hour clock shown when the system clock-format setting is 12H.
 *
 * Timers are always stored as a 0-23 hour; only the presentation and the
 * edit wheels switch to 12-hour.
 */
namespace App::TimeFormat {

/** @brief Split a 0-23 hour into a 1-12 hour and an AM/PM flag. */
inline void split12(uint8_t hour24, uint8_t& hour12, bool& pm)
{
    const SDK::Clock::Hour12 t = SDK::Clock::to12Hour(hour24);
    hour12 = t.hour;
    pm     = t.pm;
}

/** @brief Combine a 1-12 hour and an AM/PM flag back into a 0-23 hour. */
inline uint8_t to24(uint8_t hour12, bool pm)
{
    return SDK::Clock::to24Hour(hour12, pm);
}

} // namespace App::TimeFormat

#endif // TIMEFORMAT_HPP
