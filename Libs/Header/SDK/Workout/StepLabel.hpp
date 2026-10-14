/**
 ******************************************************************************
 * @file    StepLabel.hpp
 * @brief   Short text for a workout step, for lists, faces and cards.
 *
 * A step's label is its notes if it has any. Otherwise it is made from its
 * duration, the kind of step and its target, in that order, e.g.
 *
 *     400 m 3:45-4:05 /km    90 s rest    Warm up    1.5 km cool down
 *     10 min 140-155 bpm     2 mi zone 3   Open
 *
 * Distances and paces are in kilometres or miles as the runner chose. Text
 * is ASCII apart from the notes, which are shown as written.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_STEP_LABEL_HPP
#define SDK_WORKOUT_STEP_LABEL_HPP

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstddef>
#include <cstdint>

namespace SDK::Workout {

/// The step's label into @p buf, NUL-terminated and cut on a character
/// boundary if it does not fit. Returns its length.
size_t stepLabel(const Step& step, bool imperial, char* buf, size_t cap);

/// The step's duration: "400 m", "1.5 km", "0.25 mi", "90 s" (under 2
/// minutes), "5 min", "2:30", "1:05:00"; empty for an open step.
size_t stepDuration(const Step& step, bool imperial, char* buf, size_t cap);

/// The step's target: "3:45-4:05 /km", "140-155 bpm", "70-80% max",
/// "zone 3"; empty for no target.
size_t stepTarget(const Step& step, bool imperial, char* buf, size_t cap);

/// A pace for a speed: seconds per kilometre, or per mile if @p imperial,
/// rounded; 0 for a standstill.
uint32_t paceSeconds(uint32_t speedMmps, bool imperial);

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_STEP_LABEL_HPP
