#ifndef TIMERLABELS_HPP
#define TIMERLABELS_HPP

#include <Timer.hpp>
#include <texts/TextKeysAndLanguages.hpp>

namespace App::Labels
{

/// Text label for each Timer::Repeat value, indexed by the enum.
inline constexpr touchgfx::TypedTextId kRepeatLabels[Timer::REPEAT_COUNT] = {
    T_TEXT_NO,
    T_TEXT_EVERY_DAY,
    T_TEXT_WEEK_DAYS,
    T_TEXT_WEEKENDS,
    T_TEXT_MONDAY,
    T_TEXT_TUESDAY,
    T_TEXT_WEDNESDAY,
    T_TEXT_THURSDAY,
    T_TEXT_FRIDAY,
    T_TEXT_SATURDAY,
    T_TEXT_SUNDAY
};

/// Text label for each Timer::Effect value, indexed by the enum.
inline constexpr touchgfx::TypedTextId kEffectLabels[Timer::EFFECT_COUNT] = {
    T_TEXT_BEEP_AND_VIBRATE,
    T_TEXT_VIBRATE,
    T_TEXT_BEEP
};

} // namespace App::Labels

#endif // TIMERLABELS_HPP
