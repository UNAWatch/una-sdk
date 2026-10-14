/**
 ******************************************************************************
 * @file    Intervals.cpp
 * @brief   On-watch interval settings as a SDK::Workout::Program.
 ******************************************************************************
 */

#include "SDK/Workout/Intervals.hpp"

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"

#include <cstring>

namespace SDK::Workout {

namespace {

using FitDuration  = SDK::Fit::WktStepDuration;
using FitIntensity = SDK::Fit::Intensity;

uint16_t addStep(Program& p, Intensity intensity, FitIntensity fitIntensity)
{
    const uint16_t i = p.stepCount++;
    Step& s = p.steps[i];
    s = Step{};
    s.intensity       = intensity;
    s.raw.intensity   = static_cast<uint8_t>(fitIntensity);
    s.raw.targetType  = static_cast<uint8_t>(SDK::Fit::WktStepTarget::Open);
    s.raw.targetValue = 0;
    return i;
}

void setOpen(Step& s)
{
    s.end = StepEnd::Open;
    s.raw.durationType  = static_cast<uint8_t>(FitDuration::Open);
    s.raw.durationValue = 0;
}

void setPhase(Step& s, const IntervalsSpec::Phase& ph)
{
    using End = IntervalsSpec::Phase::End;
    if (ph.end == End::Time && ph.timeMs > 0) {
        s.end        = StepEnd::Time;
        s.durationMs = ph.timeMs;
        s.raw.durationType  = static_cast<uint8_t>(FitDuration::Time);
        s.raw.durationValue = ph.timeMs;
    } else if (ph.end == End::Distance && ph.distanceCm > 0) {
        s.end        = StepEnd::Distance;
        s.distanceCm = ph.distanceCm;
        s.raw.durationType  = static_cast<uint8_t>(FitDuration::Distance);
        s.raw.durationValue = ph.distanceCm;
    } else {
        setOpen(s);
    }
}

uint16_t addRepeat(Program& p, uint16_t from, uint32_t count)
{
    const uint16_t i = p.stepCount++;
    Step& s = p.steps[i];
    s = Step{};
    s.end         = StepEnd::Repeat;
    s.repeatFrom  = from;
    s.repeatCount = count;
    s.raw.durationType  = static_cast<uint8_t>(FitDuration::RepeatUntilStepsComplete);
    s.raw.durationValue = from;
    // For kRepeatForever this is also FIT's invalid value. That is harmless:
    // such a step is never written (see fitStepIndex()).
    s.raw.targetValue   = count;
    return i;
}

}  // namespace

IntervalsLayout buildIntervals(const IntervalsSpec& specIn, Program& out)
{
    IntervalsSpec spec = specIn;
    if (spec.repeats > kMaxRepeatCount) {
        spec.repeats = static_cast<uint8_t>(kMaxRepeatCount);
    }

    out = Program{};
    static constexpr char kName[] = "Intervals";
    static_assert(sizeof(kName) <= sizeof(out.name), "name fits");
    std::memcpy(out.name, kName, sizeof(kName));
    out.sport    = static_cast<uint8_t>(SDK::Fit::Sport::Running);
    out.subSport = static_cast<uint8_t>(SDK::Fit::SubSport::Generic);

    IntervalsLayout l;

    if (spec.warmUp) {
        l.warmUp = addStep(out, Intensity::Warmup, FitIntensity::Warmup);
        setOpen(out.steps[l.warmUp]);
    }

    l.run = addStep(out, Intensity::Active, FitIntensity::Active);
    setPhase(out.steps[l.run], spec.run);

    const auto addRest = [&]() {
        l.rest = addStep(out, Intensity::Rest, FitIntensity::Rest);
        setPhase(out.steps[l.rest], spec.rest);
    };

    if (spec.repeats == 0) {
        // Until the workout is ended.
        addRest();
        l.repeat = addRepeat(out, l.run, kRepeatForever);
    } else if (spec.lastRest) {
        // Every run has its rest.
        addRest();
        if (spec.repeats >= 2) {
            l.repeat = addRepeat(out, l.run, spec.repeats);
        }
    } else if (spec.repeats >= 2) {
        // The final run has no rest: (run, rest) x (repeats - 1), then a run.
        addRest();
        l.repeat   = addRepeat(out, l.run, static_cast<uint32_t>(spec.repeats - 1));
        l.finalRun = addStep(out, Intensity::Active, FitIntensity::Active);
        setPhase(out.steps[l.finalRun], spec.run);
    }
    // repeats 1 without its rest: the single run alone.

    if (spec.coolDown) {
        l.coolDown = addStep(out, Intensity::Cooldown, FitIntensity::Cooldown);
        setOpen(out.steps[l.coolDown]);
    }

    return l;
}

uint32_t intervalsRepeat(const IntervalsSpec& spec, const IntervalsLayout& layout,
                         const Engine::Status& status)
{
    const uint32_t repeats = spec.repeats > kMaxRepeatCount ? kMaxRepeatCount : spec.repeats;
    const uint16_t step = status.step;
    if (step == layout.warmUp) {
        return 0;
    }
    if (step == layout.finalRun || step == layout.coolDown) {
        // Only reached once every run is done. Unlimited repeats never get
        // here, since their block never ends.
        return repeats;
    }
    // A run or its rest: the pass of the repeat block, or 1 with no block.
    return status.rep > 0 ? status.rep : 1;
}

}  // namespace SDK::Workout
