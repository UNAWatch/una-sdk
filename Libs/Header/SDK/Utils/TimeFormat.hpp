/**
 * @file TimeFormat.hpp
 * @brief Render a time-of-day into text according to a SDK::Message::TimeFormat.
 *
 * Single source of truth for how the status-face clock is formatted across all
 * apps. Pure (ASCII, no TouchGFX dependency) so it is host-unit-testable; call
 * sites convert the result to UnicodeChar (e.g. Unicode::strncpy).
 */

#ifndef SDK_UTILS_TIMEFORMAT_HPP
#define SDK_UTILS_TIMEFORMAT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "SDK/Messages/CommandMessages.hpp"

namespace SDK::Utils
{

/**
 * @brief Format a 24-hour clock reading (@p h in 0-23, @p m in 0-59) as text.
 *
 * Writes a null-terminated ASCII string into @p out (truncated by snprintf if
 * @p outSize is too small). @p outSize must be at least 6 to hold the longest
 * valid output ("HH:MM").
 *
 *   - Hour24:        zero-padded 24-hour, colon separator  09:05, 17:42
 *   - Hour12:        1-12 hour, no leading zero, no AM/PM    9:05,  5:42
 *   - Hour24Compact: zero-padded 24-hour, no separator      0905,  1742
 *
 * Hour24 is zero-padded so it stays visually distinct from Hour12 for
 * single-digit morning hours (09:05 vs 9:05); an unpadded 24-hour reading
 * would be identical to the 12-hour one for hours 1-9. (Hours 10-12 still
 * coincide -- only an AM/PM marker could separate those, and there is none.)
 */
inline void formatTimeOfDay(char* out, std::size_t outSize,
                            std::uint8_t h, std::uint8_t m,
                            SDK::Message::TimeFormat format)
{
    const unsigned hh = static_cast<unsigned>(h);
    const unsigned mm = static_cast<unsigned>(m);

    switch (format)
    {
    case SDK::Message::TimeFormat::Hour12:
    {
        const unsigned h12 = (hh % 12u == 0u) ? 12u : (hh % 12u);
        std::snprintf(out, outSize, "%u:%02u", h12, mm);
        break;
    }
    case SDK::Message::TimeFormat::Hour24Compact:
        std::snprintf(out, outSize, "%02u%02u", hh, mm);
        break;
    case SDK::Message::TimeFormat::Hour24:
    default:
        std::snprintf(out, outSize, "%02u:%02u", hh, mm);
        break;
    }
}

} // namespace SDK::Utils

#endif // SDK_UTILS_TIMEFORMAT_HPP
