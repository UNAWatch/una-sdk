/**
 ******************************************************************************
 * @file    Timer.hpp
 * @date    25-07-2026
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
 * @brief A single countdown timer definition (duration + alert effect).
 *
 * This is the immutable configuration of a timer. The live countdown state
 * (running / paused / remaining) is tracked separately by the Service via
 * @ref TimerState and the state snapshot in Commands.hpp.
 */
struct Timer {

    /** @brief Total countdown duration in seconds (0..kMaxDurationSec). */
    uint16_t durationSec;

    /**
     * @enum Effect
     * @brief Alert effect played when the countdown reaches zero.
     */
    enum Effect : uint8_t {
        EFFECT_BEEP_AND_VIBRO,
        EFFECT_VIBRO,
        EFFECT_BEEP,

        EFFECT_COUNT    ///< Number of effect options.
    };

    Effect effect;

    /**
     * @enum Action
     * @brief Actions offered on the per-timer menu screen.
     */
    enum Action : uint8_t {
        ACTION_START = 0,   ///< Start the countdown.
        ACTION_EDIT,        ///< Edit the duration and effect.
        ACTION_DELETE,      ///< Delete the timer (recents only).
        ACTION_COUNT        ///< Number of actions.
    };

    /** @brief Upper bound for a manually entered duration: 99:59. */
    static constexpr uint16_t kMaxDurationSec = 99 * 60 + 59;

    /** @brief Identity for recents de-duplication: duration + effect. */
    bool operator==(const Timer& other) const
    {
        return durationSec == other.durationSec &&
               effect      == other.effect;
    }

    bool operator!=(const Timer& other) const
    {
        return !(*this == other);
    }
};

/**
 * @enum TimerState
 * @brief Live state of the single active countdown, owned by the Service.
 */
enum class TimerState : uint8_t {
    IDLE = 0,   ///< No countdown armed.
    RUNNING,    ///< Counting down; expires at endTick.
    PAUSED,     ///< Frozen with remainingMs left.
    FIRED       ///< Reached zero; alert is playing until acknowledged.
};

#endif // TIMER_HPP
