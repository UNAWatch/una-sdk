/**
 ******************************************************************************
 * @file    Timer.hpp
 * @date    24-07-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Timer type shared between GUI and Service.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <cstdint>

/**
 * @struct Timer
 * @brief A single timer configuration.
 */
struct Timer {

    /**
     * @enum Repeat
     * @brief Timer repetition schedule.
     */
    enum Repeat : uint8_t {
        REPEAT_NO,
        REPEAT_EVERY_DAY,
        REPEAT_WEEK_DAYS,
        REPEAT_WEEKENDS,
        REPEAT_MONDAY,
        REPEAT_TUESDAY,
        REPEAT_WEDNESDAY,
        REPEAT_THURSDAY,
        REPEAT_FRIDAY,
        REPEAT_SATURDAY,
        REPEAT_SUNDAY,

        REPEAT_COUNT    ///< Number of repeat options.
    };

    /**
     * @enum Effect
     * @brief Alert effect (sound, vibration, or both).
     */
    enum Effect : uint8_t {
        EFFECT_BEEP_AND_VIBRO,
        EFFECT_VIBRO,
        EFFECT_BEEP,

        EFFECT_COUNT    ///< Number of effect options.
    };

    /**
     * @enum Action
     * @brief Actions available on a saved timer (timer action menu).
     */
    enum Action : uint8_t {
        ACTION_TOGGLE = 0,  ///< Toggle the timer on or off.
        ACTION_EDIT,        ///< Edit the timer time and settings.
        ACTION_DELETE,      ///< Delete the timer.
        ACTION_COUNT        ///< Number of actions.
    };

    bool    on;             ///< Whether the timer is enabled.
    uint8_t timeHours;      ///< Hour component of the timer time (0–23).
    uint8_t timeMinutes;    ///< Minute component of the timer time (0–59).
    Repeat  repeat;         ///< Repetition schedule.
    Effect  effect;         ///< Alert effect.

    bool operator==(const Timer& other) const
    {
        return timeHours   == other.timeHours   &&
               timeMinutes == other.timeMinutes &&
               repeat      == other.repeat;
        // Ignore 'on' and 'effect' — they do not define the timer identity
    }

    bool operator!=(const Timer& other) const
    {
        return !(*this == other);
    }

    Timer& operator=(const Timer& other)
    {
        if (this != &other) {
            on          = other.on;
            timeHours   = other.timeHours;
            timeMinutes = other.timeMinutes;
            repeat      = other.repeat;
            effect      = other.effect;
        }
        return *this;
    }
};

#endif // TIMER_HPP
