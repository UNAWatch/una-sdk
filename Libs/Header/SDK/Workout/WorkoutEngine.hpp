/**
 ******************************************************************************
 * @file    WorkoutEngine.hpp
 * @brief   Runs a SDK::Workout::Program step by step.
 *
 * The engine walks the program's steps in file order. A repeat step sends
 * the walk back to the start of its block until the block has run
 * repeatCount times in all; blocks may nest. The engine never stops on a
 * repeat step: the current step is always one with a time, distance or
 * open end.
 *
 * It knows nothing of sensors, screens or files. The app calls update()
 * once a tick with the active time and distance since the run started,
 * and next() when the runner presses the lap button. Both report whether
 * the step changed, and how: automatically (its time or distance was
 * reached) or manually. The app decides what an alert sounds like and what
 * goes into a lap; the engine only says what happened.
 *
 * At most one step ends per update(), so a step with a zero duration lasts
 * one tick. Distance or time beyond the end of a step is not carried into
 * the next one.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_ENGINE_HPP
#define SDK_WORKOUT_ENGINE_HPP

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdint>

namespace SDK::Workout {

/// Within this long of the end of a time or distance step, Status::leadIn is set.
constexpr uint32_t kLeadInMs = 5000;

/// Below this speed a distance step's time to go is not estimated, so it
/// shows no lead-in.
constexpr uint32_t kLeadInMinSpeedMmps = 500;

/// What a whole program adds up to, repeats expanded. 64-bit throughout:
/// four nested blocks of 99 passes already run one step 96 million times.
struct Totals {
    uint64_t timeMs     = 0;      ///< Sum of the time steps.
    uint64_t distanceCm = 0;      ///< Sum of the distance steps.
    uint64_t openSteps  = 0;      ///< Open steps run, which add unknown time and distance.
    uint64_t stepsRun   = 0;      ///< Step instances, repeat steps not counted.
    bool     unlimited  = false;  ///< A kRepeatForever block: the other totals cover one pass.
};

/// Totals for @p program, which must have passed FitWorkoutReader's checks
/// (or come from an app builder that keeps the same rules).
Totals totals(const Program& program);

class Engine {
public:
    /// How a step ended.
    enum class Change : uint8_t {
        None,    ///< No step ended.
        Auto,    ///< Its time or distance was reached.
        Manual,  ///< The runner moved on (lap button, or ending the workout).
    };

    struct Status {
        bool     running  = false;  ///< A step is in progress.
        bool     finished = false;  ///< The last step has ended.

        /// The workout counts as done for the schedule: the last step has
        /// ended, or the last step is open and has started. (Whether the
        /// activity was then saved is the app's business.)
        bool reachedEnd = false;

        uint16_t step = 0;            ///< Current step's index in the program.
        uint32_t stepTimeMs = 0;      ///< Active time in the current step.
        uint32_t stepDistanceCm = 0;  ///< Distance in the current step.
        uint32_t remainingMs = 0;     ///< StepEnd::Time: time left.
        uint32_t remainingCm = 0;     ///< StepEnd::Distance: distance left.
        bool     leadIn = false;      ///< The step will end within kLeadInMs.
        bool     lastStep = false;    ///< Nothing follows the current step.

        /// The innermost repeat block holding the current step: this is pass
        /// rep of reps (reps 0 = kRepeatForever). Both 0 outside any block.
        /// On-watch Intervals number their runs differently: see
        /// intervalsRepeat() in Intervals.hpp.
        uint32_t rep  = 0;
        uint32_t reps = 0;
    };

    /// What the step that just ended was, for the app's lap.
    struct Ended {
        uint16_t step = 0;
        Change   how  = Change::None;
        uint32_t timeMs = 0;      ///< Active time it ran for.
        uint32_t distanceCm = 0;  ///< Distance it covered.
    };

    /// Start @p program at its first step. @p timeMs and @p distanceCm are
    /// the run's active totals now, normally 0. @p program must outlive the
    /// engine's use of it and must not change while it runs.
    void start(const Program& program, uint32_t timeMs, uint32_t distanceCm);

    /// Stop following the program, e.g. when the runner ends the workout.
    /// Does not end the current step: call next() first for that.
    void stop();

    /// Advance with the run's active totals. These are counted from 0 when
    /// the run starts and never go backwards; never pass a free-running
    /// clock, which can wrap. (uint32 ms covers 49 days of active time.)
    /// @p speedMmps is the current speed, used only for a distance step's
    /// lead-in: GPS speed outdoors, or e.g. a stride-derived speed on a
    /// treadmill. Pass @p speedValid false when there is none.
    Change update(uint32_t timeMs, uint32_t distanceCm, uint32_t speedMmps = 0,
                  bool speedValid = false);

    /// End the current step now, as the lap button does.
    Change next(uint32_t timeMs, uint32_t distanceCm);

    const Status& status() const { return mStatus; }

    /// The step that ended at the last change.
    const Ended& ended() const { return mEnded; }

    /// The step that will follow the current one, without moving. False if
    /// none follows.
    bool peekNext(uint16_t& step) const;

private:
    // Index of the step after leaving @p from, applying repeats to
    // @p counters; count when the program ends.
    uint16_t advance(uint16_t from, uint16_t* counters) const;
    void     enter(uint16_t step, uint32_t timeMs, uint32_t distanceCm);
    void     clearProgress();
    Change   finishStep(Change how, uint32_t timeMs, uint32_t distanceCm);
    void     refresh(uint32_t timeMs, uint32_t distanceCm, uint32_t speedMmps, bool speedValid);

    const Program* mProgram = nullptr;
    uint16_t       mCounters[kMaxSteps] = {};  ///< Passes completed, per repeat step.
    uint32_t       mStepStartMs = 0;
    uint32_t       mStepStartCm = 0;
    Status         mStatus;
    Ended          mEnded;
};

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_ENGINE_HPP
