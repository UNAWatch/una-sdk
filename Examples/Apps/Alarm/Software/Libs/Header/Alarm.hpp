/**
 ******************************************************************************
 * @file    Alarm.hpp
 * @date    24-07-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Alarm type shared between GUI and Service.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef ALARM_HPP
#define ALARM_HPP

#include <cstdint>

/**
 * @struct Alarm
 * @brief A single alarm configuration.
 */
struct Alarm {

    /**
     * @enum Repeat
     * @brief Alarm repetition schedule.
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
     * @brief Actions available on a saved alarm (alarm action menu).
     */
    enum Action : uint8_t {
        ACTION_TOGGLE = 0,  ///< Toggle the alarm on or off.
        ACTION_EDIT,        ///< Edit the alarm time and settings.
        ACTION_DELETE,      ///< Delete the alarm.
        ACTION_COUNT        ///< Number of actions.
    };

    bool    on;             ///< Whether the alarm is enabled.
    uint8_t timeHours;      ///< Hour component of the alarm time (0–23).
    uint8_t timeMinutes;    ///< Minute component of the alarm time (0–59).
    Repeat  repeat;         ///< Repetition schedule.
    Effect  effect;         ///< Alert effect.

    bool operator==(const Alarm& other) const
    {
        return timeHours   == other.timeHours   &&
               timeMinutes == other.timeMinutes &&
               repeat      == other.repeat;
        // Ignore 'on' and 'effect' — they do not define the alarm identity
    }

    bool operator!=(const Alarm& other) const
    {
        return !(*this == other);
    }

    Alarm& operator=(const Alarm& other)
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

#endif // ALARM_HPP
