/**
 ******************************************************************************
 * @file    RetroLabels.hpp
 * @brief   Text IDs the face selects between at runtime, and the one number
 *          format it needs that Unicode::snprintf does not provide.
 ******************************************************************************
 */

#ifndef RETROLABELS_HPP
#define RETROLABELS_HPP

#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Unicode.hpp>

#include <cstdint>

namespace App::Labels
{

/**
 * @brief Abbreviated day name, indexed by std::tm::tm_wday (0 = Sunday).
 *
 * Static strings rather than literals in code: the set is known at build time,
 * and keeping them in the text database leaves translation a text-database
 * job. The face reads them back with TypedText::getText() and writes them into
 * the date wildcard, which is why every glyph they can hold is also in that
 * typography's WildcardCharacters.
 */
inline constexpr touchgfx::TypedTextId kDayLabels[7] = {
    T_TEXT_SUN,
    T_TEXT_MON,
    T_TEXT_TUE,
    T_TEXT_WED,
    T_TEXT_THU,
    T_TEXT_FRI,
    T_TEXT_SAT
};

/** @brief Abbreviated month name, indexed by std::tm::tm_mon (0 = January). */
inline constexpr touchgfx::TypedTextId kMonthLabels[12] = {
    T_TEXT_JAN,
    T_TEXT_FEB,
    T_TEXT_MAR,
    T_TEXT_APR,
    T_TEXT_MAY,
    T_TEXT_JUN,
    T_TEXT_JUL,
    T_TEXT_AUG,
    T_TEXT_SEP,
    T_TEXT_OCT,
    T_TEXT_NOV,
    T_TEXT_DEC
};

/** @brief AM / PM, indexed by whether the hour is 12 or later. */
inline constexpr touchgfx::TypedTextId kMeridiemLabels[2] = {
    T_TEXT_AM,
    T_TEXT_PM
};

/**
 * @brief Write @p value in decimal, comma-grouped, e.g. 20766 -> "20,766".
 *
 * Unicode::snprintf has no grouping flag, and the design groups the step
 * count, so this is the one piece of number formatting the face does itself.
 * The result is always null-terminated; a buffer too small to hold the whole
 * number truncates from the right rather than overrunning.
 *
 * @param buf   Destination, at least one UnicodeChar.
 * @param size  Capacity of @p buf in UnicodeChars, including the terminator.
 * @param value Number to write.
 */
inline void formatGrouped(touchgfx::Unicode::UnicodeChar *buf,
                          uint16_t size,
                          uint32_t value)
{
    if (!buf || (size == 0)) {
        return;
    }

    // Least significant digit first, which is the order division gives them.
    char digits[10];
    uint8_t count = 0;
    do {
        digits[count++] = static_cast<char>('0' + (value % 10u));
        value /= 10u;
    } while ((value != 0u) && (count < sizeof(digits)));

    uint16_t out = 0;
    for (uint8_t i = count; (i > 0) && ((out + 1u) < size); i--) {
        buf[out++] = static_cast<touchgfx::Unicode::UnicodeChar>(digits[i - 1]);

        // A separator goes after this digit when the number of digits still to
        // come is a non-zero multiple of three.
        const uint8_t remaining = static_cast<uint8_t>(i - 1);
        if ((remaining != 0u) && ((remaining % 3u) == 0u) && ((out + 1u) < size)) {
            buf[out++] = static_cast<touchgfx::Unicode::UnicodeChar>(',');
        }
    }

    buf[out] = 0;
}

} // namespace App::Labels

#endif // RETROLABELS_HPP
