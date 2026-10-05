/**
 ******************************************************************************
 * @file    Intervals.hpp
 * @brief   Builds a SDK::Workout::Program from simple on-watch interval
 *          settings, so they run on SDK::Workout::Engine like any workout.
 *
 * The program is an optional open warm-up, then a run and a rest repeated
 * a number of times, then an optional open cool-down. With lastRest false
 * the final rest is left out, so the last run leads straight to the
 * cool-down. With repeats 0 the run and rest repeat until the workout is
 * ended. That block uses kRepeatForever, which a FIT file cannot hold, so a
 * recording must write it without its repeat step.
 *
 * The steps are laid out as a FIT workout would be, so the same program can
 * be written to an activity file:
 *
 *   warm-up?  run  [rest  repeat->run]  [final run]  cool-down?
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_INTERVALS_HPP
#define SDK_WORKOUT_INTERVALS_HPP

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdint>

namespace SDK::Workout {

struct IntervalsSpec {
    /// How a run or a rest ends. A zero time or distance means open.
    struct Phase {
        enum class End : uint8_t { Open, Time, Distance } end = End::Open;
        uint32_t timeMs     = 0;
        uint32_t distanceCm = 0;
    };

    uint8_t repeats  = 0;     ///< Run-rest cycles; 0 = until the workout is ended.
    Phase   run;
    Phase   rest;
    bool    warmUp   = true;  ///< Open warm-up first.
    bool    coolDown = true;  ///< Open cool-down last.
    bool    lastRest = true;  ///< Rest after the final run; ignored when repeats is 0.
};

/// Where the converter put each kind of step, by program index. kNone if
/// the program has no such step.
struct IntervalsLayout {
    static constexpr uint16_t kNone = 0xFFFF;
    uint16_t warmUp   = kNone;
    uint16_t run      = kNone;  ///< The run inside the repeat block.
    uint16_t rest     = kNone;
    uint16_t repeat   = kNone;  ///< The repeat step.
    uint16_t finalRun = kNone;  ///< Standalone final run (lastRest false, repeats >= 2).
    uint16_t coolDown = kNone;
};

/// Fill @p out with the program for @p spec, named "Intervals". Returns the
/// layout, so the app can tell which kind of step is running.
IntervalsLayout buildIntervals(const IntervalsSpec& spec, Program& out);

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_INTERVALS_HPP
