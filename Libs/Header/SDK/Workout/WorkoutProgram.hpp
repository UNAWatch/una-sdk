/**
 ******************************************************************************
 * @file    WorkoutProgram.hpp
 * @brief   A structured workout as a fixed-size list of steps.
 *
 * The decoded form of a FIT workout file (see SDK::Fit::FitWorkoutReader).
 * Steps are kept in file order, indexed by their FIT message_index, with
 * repeat steps left in place rather than expanded. A program is a single
 * fixed-size object: it does not allocate, so one instance can be reused
 * for every file an app reads.
 *
 * Each step carries both the interpreted meaning the watch acts on and the
 * raw values from the file. The raw values let a recording copy the
 * workout into an activity file unchanged, including steps the watch could
 * only run in a simplified form (see Step::degraded).
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_PROGRAM_HPP
#define SDK_WORKOUT_PROGRAM_HPP

#include <cstddef>
#include <cstdint>

namespace SDK::Workout {

constexpr uint16_t kMaxSteps          = 100;  ///< Steps per workout, repeat steps included.
constexpr uint8_t  kMaxRepeatDepth    = 4;    ///< Repeat blocks nested inside each other.
constexpr uint32_t kMaxRepeatCount    = 99;   ///< Larger counts are clamped and marked degraded.
constexpr size_t   kNameBytes         = 64;   ///< Workout name, NUL included.
constexpr size_t   kDescriptionBytes  = 512;  ///< Workout description, NUL included.
constexpr size_t   kStepNotesBytes    = 64;   ///< Step notes, NUL included.
constexpr size_t   kStepNameBytes     = 32;   ///< Step name, NUL included.

/// What ends a step.
enum class StepEnd : uint8_t {
    Time,      ///< After durationMs of active time.
    Distance,  ///< After distanceCm of distance.
    Open,      ///< Only when the runner moves on (lap button).
    Repeat,    ///< Not a step to run: jumps back to repeatFrom, repeatCount times in all.
};

/// What a step is aimed at.
enum class Target : uint8_t {
    Open,       ///< No target.
    Speed,      ///< speedLowMmps .. speedHighMmps.
    HeartRate,  ///< See HeartRateTarget.
};

/// How a heart-rate target is given in the file.
enum class HeartRateTarget : uint8_t {
    Zone,        ///< hrZone, 1-5, resolved against the watch's own zones.
    Bpm,         ///< hrLow .. hrHigh beats per minute.
    PercentMax,  ///< hrLow .. hrHigh percent of maximum heart rate.
};

/// The kind of effort a step is.
enum class Intensity : uint8_t {
    Active,
    Rest,
    Warmup,
    Cooldown,
};

struct Step {
    // Members are ordered by size to keep the struct free of padding.
    uint32_t durationMs    = 0;  ///< StepEnd::Time.
    uint32_t distanceCm    = 0;  ///< StepEnd::Distance.
    uint32_t repeatCount   = 0;  ///< StepEnd::Repeat: times the block runs in all, 1 to kMaxRepeatCount.
    uint32_t speedLowMmps  = 0;  ///< Target::Speed, mm/s.
    uint32_t speedHighMmps = 0;  ///< Target::Speed, mm/s.
    uint16_t repeatFrom    = 0;  ///< StepEnd::Repeat: index of the first step of the block.
    uint16_t hrLow         = 0;  ///< Target::HeartRate: bpm or percent, by hrKind.
    uint16_t hrHigh        = 0;  ///< Target::HeartRate: bpm or percent, by hrKind.

    StepEnd         end       = StepEnd::Open;
    Target          target    = Target::Open;
    Intensity       intensity = Intensity::Active;
    HeartRateTarget hrKind    = HeartRateTarget::Zone;
    uint8_t         hrZone    = 0;  ///< HeartRateTarget::Zone, 1-5.

    /// True when the file asked for something the watch cannot do and the
    /// step was simplified: an unsupported target became Open, an
    /// unsupported end became Open, an unsupported repeat runs its body
    /// once, or a repeat count was clamped.
    bool degraded = false;

    char notes[kStepNotesBytes] = {};  ///< UTF-8, may be empty.
    char name[kStepNameBytes]   = {};  ///< UTF-8, may be empty.

    /// Values exactly as read from the file, for writing the workout back
    /// out. A field the file left out holds its FIT invalid value.
    struct Raw {
        uint32_t durationValue = 0xFFFFFFFFu;
        uint32_t targetValue   = 0xFFFFFFFFu;
        uint32_t customLow     = 0xFFFFFFFFu;
        uint32_t customHigh    = 0xFFFFFFFFu;
        uint8_t  durationType  = 0xFF;
        uint8_t  targetType    = 0xFF;
        uint8_t  intensity     = 0xFF;
    } raw;
};

/// About 15 KB, nearly all of it the step notes and names. Keep one
/// long-lived instance, like the reader, never a local.
struct Program {
    char     name[kNameBytes]               = {};  ///< UTF-8, may be empty.
    char     description[kDescriptionBytes] = {};  ///< UTF-8, may be empty.
    uint8_t  sport     = 0xFF;  ///< FIT sport; 0xFF if the file left it out.
    uint8_t  subSport  = 0xFF;  ///< FIT sub_sport; 0xFF if the file left it out.
    uint16_t stepCount = 0;     ///< Valid entries in steps.
    bool     degraded  = false; ///< At least one step is degraded.
    Step     steps[kMaxSteps];
};

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_PROGRAM_HPP
