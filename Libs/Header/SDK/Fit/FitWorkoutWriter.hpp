/**
 ******************************************************************************
 * @file    FitWorkoutWriter.hpp
 * @brief   Writes a SDK::Workout::Program into a FIT file as workout,
 *          workout_step and memo_glob messages.
 *
 * The counterpart of FitWorkoutReader: an activity file can carry the
 * workout it followed, and a workout file can be written from a program.
 * Each step is written from its raw values (Step::raw), so a step the watch
 * ran in a simplified form keeps the duration and target the file asked for.
 *
 * Steps that repeat until the workout is ended (kRepeatForever) cannot be
 * expressed in FIT and are left out; the other steps are renumbered to
 * match, as fitStepIndex() describes. Use fitStepIndex() for each lap's
 * wkt_step_index as well.
 *
 * wkt_description holds at most kWktDescriptionBytes. A longer description
 * is also written whole as memo_glob parts, which FitWorkoutReader prefers.
 *
 * The caller has begun the file and written file_id. Messages go to the
 * local types it names, which this function redefines.
 ******************************************************************************
 */

#ifndef SDK_FIT_WORKOUT_WRITER_HPP
#define SDK_FIT_WORKOUT_WRITER_HPP

#include "SDK/Fit/FitWriter.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdint>

namespace SDK::Fit {

/// Bytes of the description written in wkt_description, NUL included.
constexpr uint8_t kWktDescriptionBytes = 160;

/// Bytes of text in each memo_glob part.
constexpr uint8_t kMemoPartBytes = 250;

/// Local message types for the workout messages.
struct WorkoutLocalTypes {
    uint8_t workout;
    uint8_t step;
    uint8_t memo;  ///< Used only when the description needs memo_glob parts.
};

/// Write @p program to @p fit. Returns false if any message failed to write.
bool writeWorkout(FitWriter& fit, const SDK::Workout::Program& program,
                  const WorkoutLocalTypes& local);

}  // namespace SDK::Fit

#endif  // SDK_FIT_WORKOUT_WRITER_HPP
