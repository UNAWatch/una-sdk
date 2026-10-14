/**
 ******************************************************************************
 * @file    WorkoutAlerts.hpp
 * @brief   When to alert while a SDK::Workout::Engine runs a program.
 *
 * The engine reports what is happening; these classes decide when that is
 * worth a beep. They know nothing of buzzers or screens: the app asks them
 * once per active tick and plays whatever pattern it likes.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_ALERTS_HPP
#define SDK_WORKOUT_ALERTS_HPP

#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"

#include <cstdint>

namespace SDK::Workout {

/**
 * A beep each second as a time or distance step nears its end.
 *
 * It starts on the first tick the engine sets Status::leadIn, so about
 * kLeadInMs before the end, and then beeps every tick until the step ends.
 * A distance step's lead-in is estimated from the current speed, which can
 * waver or drop out for a second; once started, the beeps carry on
 * regardless, so the countdown keeps its rhythm. They stop after kMaxBeeps,
 * so a runner who stops just short of the distance is not beeped at
 * indefinitely.
 *
 * Call stepStarted() on every step change, and tick() on every other active
 * tick: the tick that changes step has its own alert. Paused seconds are not
 * ticks.
 */
class LeadInAlert {
public:
    /// Beeps at most per step: the lead-in's 5 at a steady pace, and some
    /// slack for a distance step whose estimate was early.
    static constexpr uint8_t kMaxBeeps = 7;

    /// A new step has started (also call it when the program starts).
    void stepStarted();

    /// One active tick with the engine's status after update(). True if
    /// this tick should beep.
    bool tick(const Engine::Status& status);

private:
    bool    mStarted = false;
    uint8_t mBeeps   = 0;
};

/**
 * When a step's target zone is worth an alert, from a target gauge's zone.
 *
 *   - Leaving the band: an alert saying which way, then again every
 *     kRepeatSec while still outside it, or at once if it crosses to the
 *     other side.
 *   - Coming back in after an out-of-band alert: one BackIn alert. A step
 *     that starts in the band stays quiet, and so does a step that leaves it
 *     only while alerts are held back.
 *   - Held back: in the first kQuietStartSec of a step, and while the zone
 *     is Unknown (the gauge's hold-off, or no target) or NoSignal. An alert
 *     still outstanding when the signal returns is kept, so coming back in
 *     afterwards still says so.
 *
 * Call stepStarted() on every step change, and tick() once per active tick
 * with the gauge's zone after its own tick(). Paused seconds are not ticks.
 */
class ZoneAlerts {
public:
    enum class Alert : uint8_t {
        None,
        Above,   ///< Too fast, or heart rate too high.
        Below,   ///< Too slow, or heart rate too low.
        BackIn,  ///< Back in the band after an Above or Below alert.
    };

    static constexpr uint32_t kQuietStartSec = 20;
    static constexpr uint32_t kRepeatSec     = 30;

    /// A new step has started (also call it when the program starts).
    void stepStarted();

    /// One active tick with the gauge's zone. The alert to play, if any.
    Alert tick(Zone zone);

private:
    uint32_t mStepSec    = 0;
    uint32_t mSinceAlert = 0;
    Zone     mAlerted    = Zone::Unknown;  ///< Above or Below while an alert is outstanding.
};

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_ALERTS_HPP
