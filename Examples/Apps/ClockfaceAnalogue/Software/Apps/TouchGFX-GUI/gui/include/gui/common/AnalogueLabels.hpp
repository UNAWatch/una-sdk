/**
 ******************************************************************************
 * @file    AnalogueLabels.hpp
 * @date    27-August-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Text IDs the face selects between at runtime.
 ******************************************************************************
 */

#ifndef ANALOGUELABELS_HPP
#define ANALOGUELABELS_HPP

#include <texts/TextKeysAndLanguages.hpp>

namespace App::Labels
{

/**
 * @brief Abbreviated day name, indexed by std::tm::tm_wday (0 = Sunday).
 *
 * Seven static strings rather than one wildcard: the set is known at build
 * time, so the face swaps the TextArea's typed text instead of writing
 * characters into a RAM buffer, and translation stays a text-database job.
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

} // namespace App::Labels

#endif // ANALOGUELABELS_HPP
