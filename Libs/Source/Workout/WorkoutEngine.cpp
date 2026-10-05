/**
 ******************************************************************************
 * @file    WorkoutEngine.cpp
 * @brief   Step-by-step execution of a SDK::Workout::Program.
 ******************************************************************************
 */

#include "SDK/Workout/WorkoutEngine.hpp"

#include <cstring>

namespace SDK::Workout {

namespace {

// Repeat blocks a step sits in are the repeat steps after it whose block
// starts at or before it. FitWorkoutReader guarantees blocks nest, so these
// are exactly the step's enclosing blocks, innermost first.
bool encloses(const Step& repeat, uint16_t repeatIndex, uint16_t step)
{
    return repeat.end == StepEnd::Repeat && repeatIndex > step && repeat.repeatFrom <= step;
}

}  // namespace

Totals totals(const Program& program)
{
    Totals t;
    for (uint16_t i = 0; i < program.stepCount; ++i) {
        const Step& s = program.steps[i];
        if (s.end == StepEnd::Repeat) {
            continue;
        }
        uint64_t passes = 1;
        for (uint16_t r = i + 1; r < program.stepCount; ++r) {
            const Step& rep = program.steps[r];
            if (!encloses(rep, r, i)) {
                continue;
            }
            if (rep.repeatCount == kRepeatForever) {
                t.unlimited = true;
            } else {
                passes *= rep.repeatCount;
            }
        }
        t.stepsRun += static_cast<uint32_t>(passes);
        if (s.end == StepEnd::Time) {
            t.timeMs += passes * s.durationMs;
        } else if (s.end == StepEnd::Distance) {
            t.distanceCm += passes * s.distanceCm;
        } else {
            t.openSteps += static_cast<uint32_t>(passes);
        }
    }
    return t;
}

uint16_t Engine::advance(uint16_t from, uint16_t* counters) const
{
    const uint16_t count = mProgram->stepCount;
    uint16_t pos = static_cast<uint16_t>(from + 1);

    // A valid program reaches a real step after passing at most a few repeat
    // steps. The bound only stops a malformed one from looping forever.
    for (uint32_t guard = 0; pos < count && guard < 4u * kMaxSteps; ++guard) {
        const Step& s = mProgram->steps[pos];
        if (s.end != StepEnd::Repeat) {
            return pos;
        }
        uint16_t& done = counters[pos];
        const bool again = s.repeatCount == kRepeatForever ||
                           static_cast<uint32_t>(done) + 1 < s.repeatCount;
        if (again) {
            if (done < UINT16_MAX) {
                ++done;
            }
            pos = s.repeatFrom;
        } else {
            done = 0;  // the block is finished; an outer pass starts it afresh
            ++pos;
        }
    }
    return count;
}

void Engine::start(const Program& program, uint32_t timeMs, uint32_t distanceCm)
{
    mProgram = &program;
    std::memset(mCounters, 0, sizeof(mCounters));
    mStatus = Status{};
    mEnded  = Ended{};
    // A repeat must point back to an earlier step, so a valid program never
    // starts with one. Treat either case as nothing to run.
    if (program.stepCount == 0 || program.steps[0].end == StepEnd::Repeat) {
        mStatus.finished = true;
        return;
    }
    enter(0, timeMs, distanceCm);
}

void Engine::stop()
{
    mStatus.running = false;
}

void Engine::enter(uint16_t step, uint32_t timeMs, uint32_t distanceCm)
{
    mStatus.running = true;
    mStatus.step    = step;
    mStepStartMs    = timeMs;
    mStepStartCm    = distanceCm;
    refresh(timeMs, distanceCm, 0, false);
    if (mStatus.lastStep && mProgram->steps[step].end == StepEnd::Open) {
        mStatus.reachedEnd = true;
    }
}

void Engine::refresh(uint32_t timeMs, uint32_t distanceCm, uint32_t speedMmps, bool speedValid)
{
    const Step& s = mProgram->steps[mStatus.step];
    Status& st = mStatus;

    st.stepTimeMs     = timeMs >= mStepStartMs ? timeMs - mStepStartMs : 0;
    st.stepDistanceCm = distanceCm >= mStepStartCm ? distanceCm - mStepStartCm : 0;
    st.remainingMs = 0;
    st.remainingCm = 0;
    st.leadIn = false;

    if (s.end == StepEnd::Time) {
        st.remainingMs = st.stepTimeMs < s.durationMs ? s.durationMs - st.stepTimeMs : 0;
        st.leadIn = st.remainingMs > 0 && st.remainingMs <= kLeadInMs;
    } else if (s.end == StepEnd::Distance) {
        st.remainingCm = st.stepDistanceCm < s.distanceCm ? s.distanceCm - st.stepDistanceCm : 0;
        if (speedValid && speedMmps >= kLeadInMinSpeedMmps && st.remainingCm > 0) {
            // Time to go = remaining / speed; compare in mm and ms.
            const uint64_t remainingMm = static_cast<uint64_t>(st.remainingCm) * 10u;
            st.leadIn = remainingMm * 1000u <= static_cast<uint64_t>(kLeadInMs) * speedMmps;
        }
    }

    st.rep  = 0;
    st.reps = 0;
    for (uint16_t r = static_cast<uint16_t>(st.step + 1); r < mProgram->stepCount; ++r) {
        const Step& rep = mProgram->steps[r];
        if (encloses(rep, r, st.step)) {
            st.rep  = static_cast<uint32_t>(mCounters[r]) + 1;
            st.reps = rep.repeatCount == kRepeatForever ? 0 : rep.repeatCount;
            break;  // the first enclosing block found is the innermost
        }
    }

    uint16_t following = 0;
    st.lastStep = !peekNext(following);
}

Engine::Change Engine::finishStep(Change how, uint32_t timeMs, uint32_t distanceCm)
{
    mEnded.step       = mStatus.step;
    mEnded.how        = how;
    mEnded.timeMs     = timeMs >= mStepStartMs ? timeMs - mStepStartMs : 0;
    mEnded.distanceCm = distanceCm >= mStepStartCm ? distanceCm - mStepStartCm : 0;

    const uint16_t following = advance(mStatus.step, mCounters);
    if (following >= mProgram->stepCount) {
        mStatus.running    = false;
        mStatus.finished   = true;
        mStatus.reachedEnd = true;
        mStatus.leadIn     = false;
        return how;
    }
    enter(following, timeMs, distanceCm);
    return how;
}

Engine::Change Engine::update(uint32_t timeMs, uint32_t distanceCm, uint32_t speedMmps,
                              bool speedValid)
{
    if (!mStatus.running) {
        return Change::None;
    }
    refresh(timeMs, distanceCm, speedMmps, speedValid);

    const Step& s = mProgram->steps[mStatus.step];
    const bool done = (s.end == StepEnd::Time && mStatus.stepTimeMs >= s.durationMs) ||
                      (s.end == StepEnd::Distance && mStatus.stepDistanceCm >= s.distanceCm);
    if (!done) {
        return Change::None;
    }
    const Change how = finishStep(Change::Auto, timeMs, distanceCm);
    if (mStatus.running) {
        refresh(timeMs, distanceCm, speedMmps, speedValid);
    }
    return how;
}

Engine::Change Engine::next(uint32_t timeMs, uint32_t distanceCm)
{
    if (!mStatus.running) {
        return Change::None;
    }
    return finishStep(Change::Manual, timeMs, distanceCm);
}

bool Engine::peekNext(uint16_t& step) const
{
    if (!mStatus.running || mProgram == nullptr) {
        return false;
    }
    uint16_t counters[kMaxSteps];
    std::memcpy(counters, mCounters, sizeof(counters));
    const uint16_t following = advance(mStatus.step, counters);
    if (following >= mProgram->stepCount) {
        return false;
    }
    step = following;
    return true;
}

}  // namespace SDK::Workout
