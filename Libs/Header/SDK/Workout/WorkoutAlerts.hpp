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

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_ALERTS_HPP
