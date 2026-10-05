/**
 ******************************************************************************
 * @file    WorkoutEngine_test.cpp
 * @brief   Tests for SDK::Workout::Engine, totals() and buildIntervals().
 ******************************************************************************
 */

#include "SDK/Workout/Intervals.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace SDK::Workout;
using Change = Engine::Change;

namespace {

// Builds programs step by step, the way FitWorkoutReader leaves them.
struct P {
    std::unique_ptr<Program> p = std::make_unique<Program>();

    P& time(uint32_t ms, Intensity in = Intensity::Active)
    {
        Step& s = add(in);
        s.end = StepEnd::Time;
        s.durationMs = ms;
        return *this;
    }
    P& dist(uint32_t cm, Intensity in = Intensity::Active)
    {
        Step& s = add(in);
        s.end = StepEnd::Distance;
        s.distanceCm = cm;
        return *this;
    }
    P& open(Intensity in = Intensity::Active)
    {
        add(in).end = StepEnd::Open;
        return *this;
    }
    P& repeat(uint16_t from, uint32_t count)
    {
        Step& s = add(Intensity::Active);
        s.end = StepEnd::Repeat;
        s.repeatFrom = from;
        s.repeatCount = count;
        return *this;
    }
    Step& add(Intensity in)
    {
        Step& s = p->steps[p->stepCount++];
        s = Step{};
        s.intensity = in;
        return s;
    }
    const Program& operator*() const { return *p; }
};

// The 400 m session: (400 m + 60 s) x5, then 60 s, the whole block x2.
P fourHundreds()
{
    P b;
    b.open(Intensity::Warmup)                 // 0
        .time(90000, Intensity::Rest)         // 1
        .dist(40000)                          // 2
        .time(60000, Intensity::Rest)         // 3
        .repeat(2, 5)                         // 4
        .time(60000, Intensity::Rest)         // 5
        .repeat(2, 2)                         // 6
        .dist(150000, Intensity::Cooldown);   // 7
    return b;
}

// Run @p prog to the end, ending every step as soon as it can: time and
// distance steps automatically, open steps by hand. Records the steps run.
struct Walk {
    std::vector<uint16_t>    steps;
    std::vector<std::string> reps;   // "k/N" per step entered, "" outside any block
    std::vector<Change>      how;
};

Walk runAll(const Program& prog, size_t limit = 500)
{
    Engine e;
    Walk r;
    uint32_t t = 0, d = 0;
    e.start(prog, t, d);
    while (e.status().running && r.steps.size() < limit) {
        const Engine::Status& st = e.status();
        r.steps.push_back(st.step);
        r.reps.push_back(st.reps || st.rep ? std::to_string(st.rep) + "/" + std::to_string(st.reps)
                                           : std::string());
        const Step& s = prog.steps[st.step];
        Change c;
        if (s.end == StepEnd::Time) {
            t += s.durationMs;
            c = e.update(t, d);
        } else if (s.end == StepEnd::Distance) {
            d += s.distanceCm;
            c = e.update(t, d);
        } else {
            c = e.next(t, d);
        }
        r.how.push_back(c);
    }
    return r;
}

}  // namespace

// --- Walking the program -------------------------------------------------------

TEST(WorkoutEngine, NestedRepeatsRunTwentyFiveSteps)
{
    const P prog = fourHundreds();
    const Walk r = runAll(*prog);
    const std::vector<uint16_t> want = {0, 1,
        2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 5,
        2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 5,
        7};
    EXPECT_EQ(r.steps, want);
}

TEST(WorkoutEngine, RepLabelsFollowTheInnermostBlock)
{
    const P prog = fourHundreds();
    const Walk r = runAll(*prog);
    const std::vector<std::string> want = {"", "",
        "1/5", "1/5", "2/5", "2/5", "3/5", "3/5", "4/5", "4/5", "5/5", "5/5", "1/2",
        "1/5", "1/5", "2/5", "2/5", "3/5", "3/5", "4/5", "4/5", "5/5", "5/5", "2/2",
        ""};
    EXPECT_EQ(r.reps, want);
}

TEST(WorkoutEngine, SimpleRepeat)
{
    P prog;
    prog.open(Intensity::Warmup).dist(100000).dist(100000).repeat(1, 3).time(90000, Intensity::Rest);
    EXPECT_EQ(runAll(*prog).steps, (std::vector<uint16_t>{0, 1, 2, 1, 2, 1, 2, 4}));
}

TEST(WorkoutEngine, RepeatCountOneRunsOnce)
{
    P prog;
    prog.time(1000).repeat(0, 1).time(2000);
    EXPECT_EQ(runAll(*prog).steps, (std::vector<uint16_t>{0, 2}));
}

TEST(WorkoutEngine, EndsAndReportsHow)
{
    P prog;
    prog.time(60000).dist(40000).open();
    Engine e;
    e.start(*prog, 1000, 500);  // a run already under way
    EXPECT_EQ(e.update(60999, 500), Change::None);
    EXPECT_EQ(e.status().remainingMs, 1u);
    EXPECT_EQ(e.update(61000, 600), Change::Auto);
    EXPECT_EQ(e.ended().step, 0);
    EXPECT_EQ(e.ended().how, Change::Auto);
    EXPECT_EQ(e.ended().timeMs, 60000u);
    EXPECT_EQ(e.ended().distanceCm, 100u);
    EXPECT_EQ(e.status().step, 1);

    // Ended early by hand.
    EXPECT_EQ(e.next(70000, 20600), Change::Manual);
    EXPECT_EQ(e.ended().step, 1);
    EXPECT_EQ(e.ended().distanceCm, 20000u);
    EXPECT_EQ(e.status().step, 2);

    // Open: only next() ends it.
    EXPECT_EQ(e.update(999999, 999999), Change::None);
    EXPECT_EQ(e.next(1000000, 1000000), Change::Manual);
    EXPECT_FALSE(e.status().running);
    EXPECT_TRUE(e.status().finished);
    EXPECT_EQ(e.update(1000001, 1000001), Change::None);
    EXPECT_EQ(e.next(1000001, 1000001), Change::None);
}

TEST(WorkoutEngine, OvershootIsNotCarriedOver)
{
    P prog;
    prog.dist(40000).dist(40000);
    Engine e;
    e.start(*prog, 0, 0);
    EXPECT_EQ(e.update(100000, 40500), Change::Auto);  // 5 m past the end
    EXPECT_EQ(e.status().stepDistanceCm, 0u);
    EXPECT_EQ(e.status().remainingCm, 40000u);
}

TEST(WorkoutEngine, AtMostOneStepEndsPerUpdate)
{
    P prog;
    prog.time(0).time(0).time(1000);
    Engine e;
    e.start(*prog, 0, 0);
    EXPECT_EQ(e.update(5000, 0), Change::Auto);
    EXPECT_EQ(e.status().step, 1);
    EXPECT_EQ(e.update(5000, 0), Change::Auto);
    EXPECT_EQ(e.status().step, 2);
}

TEST(WorkoutEngine, StopLeavesTheStep)
{
    P prog;
    prog.time(60000).time(60000);
    Engine e;
    e.start(*prog, 0, 0);
    e.stop();
    EXPECT_FALSE(e.status().running);
    EXPECT_FALSE(e.status().finished);
    EXPECT_EQ(e.update(120000, 0), Change::None);
}

// --- What the face shows ---------------------------------------------------------

TEST(WorkoutEngine, LeadInOnTimeSteps)
{
    P prog;
    prog.time(60000).open();
    Engine e;
    e.start(*prog, 0, 0);
    e.update(54999, 0);
    EXPECT_FALSE(e.status().leadIn);
    e.update(55000, 0);
    EXPECT_TRUE(e.status().leadIn);
    e.update(59999, 0);
    EXPECT_TRUE(e.status().leadIn);
    e.update(60000, 0);  // the next step is open: no lead-in there
    EXPECT_FALSE(e.status().leadIn);
}

TEST(WorkoutEngine, LeadInOnDistanceStepsNeedsASpeed)
{
    P prog;
    prog.dist(40000);
    Engine e;
    e.start(*prog, 0, 0);
    // 20 m to go at 4 m/s is 5 s.
    e.update(0, 38000, 4000, true);
    EXPECT_TRUE(e.status().leadIn);
    e.update(0, 37900, 4000, true);  // 21 m: 5.25 s
    EXPECT_FALSE(e.status().leadIn);
    e.update(0, 38000, 4000, false);  // no current GPS speed
    EXPECT_FALSE(e.status().leadIn);
    e.update(0, 39900, 400, true);  // below the speed floor
    EXPECT_FALSE(e.status().leadIn);
}

TEST(WorkoutEngine, PeekAndLastStep)
{
    const P prog = fourHundreds();
    Engine e;
    e.start(*prog, 0, 0);
    uint16_t following = 0;
    ASSERT_TRUE(e.peekNext(following));
    EXPECT_EQ(following, 1);
    e.next(0, 0);
    e.next(0, 0);
    ASSERT_TRUE(e.peekNext(following));
    EXPECT_EQ(following, 3);
    e.next(0, 0);  // now step 3, the end of the inner block
    ASSERT_TRUE(e.peekNext(following));
    EXPECT_EQ(following, 2);  // back round, and peeking moved nothing
    EXPECT_EQ(e.status().rep, 1u);
    EXPECT_FALSE(e.status().lastStep);
}

TEST(WorkoutEngine, ReachedEndOnStartingAnOpenLastStep)
{
    P prog;
    prog.time(1000).open(Intensity::Cooldown);
    Engine e;
    e.start(*prog, 0, 0);
    EXPECT_FALSE(e.status().reachedEnd);
    e.update(1000, 0);
    EXPECT_TRUE(e.status().lastStep);
    EXPECT_TRUE(e.status().reachedEnd);  // the open cool-down has started
    EXPECT_FALSE(e.status().finished);
    e.next(2000, 0);
    EXPECT_TRUE(e.status().finished);
}

TEST(WorkoutEngine, ReachedEndOnlyAfterATimedLastStep)
{
    P prog;
    prog.time(1000).time(1000);
    Engine e;
    e.start(*prog, 0, 0);
    e.update(1000, 0);
    EXPECT_TRUE(e.status().lastStep);
    EXPECT_FALSE(e.status().reachedEnd);
    e.update(2000, 0);
    EXPECT_TRUE(e.status().reachedEnd);
    EXPECT_TRUE(e.status().finished);
}

TEST(WorkoutEngine, ForeverNeverReachesTheEnd)
{
    P prog;
    prog.time(1000).time(1000, Intensity::Rest).repeat(0, kRepeatForever).open(Intensity::Cooldown);
    const Walk r = runAll(*prog, 41);
    ASSERT_EQ(r.steps.size(), 41u);
    EXPECT_EQ(r.steps[40], 0);
    EXPECT_EQ(r.reps[40], "21/0");
    Engine e;
    e.start(*prog, 0, 0);
    EXPECT_FALSE(e.status().lastStep);
    EXPECT_EQ(e.status().reps, 0u);
}

TEST(WorkoutEngine, EmptyProgramIsFinished)
{
    Program empty;
    Engine e;
    e.start(empty, 0, 0);
    EXPECT_FALSE(e.status().running);
    EXPECT_TRUE(e.status().finished);
}

// --- Totals ----------------------------------------------------------------------

TEST(WorkoutEngine, TotalsExpandRepeats)
{
    const Totals t = totals(*fourHundreds());
    EXPECT_EQ(t.timeMs, (90u + (60u * 5 + 60u) * 2) * 1000u);  // 810 s
    EXPECT_EQ(t.distanceCm, (400u * 10 + 1500u) * 100u);       // 5.5 km
    EXPECT_EQ(t.openSteps, 1u);
    EXPECT_EQ(t.stepsRun, 25u);
    EXPECT_FALSE(t.unlimited);
}

TEST(WorkoutEngine, TotalsFlagForever)
{
    P prog;
    prog.time(1000).repeat(0, kRepeatForever);
    const Totals t = totals(*prog);
    EXPECT_TRUE(t.unlimited);
    EXPECT_EQ(t.timeMs, 1000u);
}

// --- Intervals: the same phases as the Running app's state machine -------------

namespace {

// The Running app's Intervals state machine (Service.cpp: startIntervalsPhase,
// advanceIntervalsPhase, emitIntervalsWorkout, intervalsWktStepIndex), reduced
// to the phases it runs and the FIT step index each one records.
struct RunningIntervals {
    enum Phase { WARM_UP, RUN, REST, COOL_DOWN };
    enum Metric { OPEN, TIME, DISTANCE };
    struct Cfg {
        uint8_t repeatsNum;
        Metric  runMetric;  uint32_t runTime;  float runDistance;
        Metric  restMetric; uint32_t restTime; float restDistance;
        bool    warmUp, coolDown, lastRest;
    } cfg;
    Phase phase = WARM_UP;
    int   repeat = 0;
    bool  completed = false;

    void start()
    {
        if (cfg.warmUp) {
            phase = WARM_UP;
        } else {
            repeat = 1;
            phase = RUN;
        }
    }
    void advance()
    {
        switch (phase) {
        case WARM_UP: phase = RUN; repeat = 1; break;
        case RUN:
            if (!cfg.lastRest && cfg.repeatsNum != 0 && repeat >= cfg.repeatsNum) {
                if (cfg.coolDown) phase = COOL_DOWN; else completed = true;
            } else {
                phase = REST;
            }
            break;
        case REST:
            if (cfg.repeatsNum == 0 || repeat < cfg.repeatsNum) {
                repeat++;
                phase = RUN;
            } else {
                if (cfg.coolDown) phase = COOL_DOWN; else completed = true;
            }
            break;
        case COOL_DOWN: completed = true; break;
        }
    }
    // The FIT step index the current phase's lap records: the step map
    // emitIntervalsWorkout() builds, read as intervalsWktStepIndex() does.
    int wktStepIndex() const
    {
        int n = 0;
        const int warmUpIdx = cfg.warmUp ? n++ : -1;
        const int runIdx = n++;
        int restIdx = -1, finalRunIdx = runIdx;
        if (cfg.repeatsNum == 0) {
            restIdx = n++;                         // no repeat step is written
        } else if (cfg.lastRest) {
            restIdx = n++;
            if (cfg.repeatsNum >= 2) n++;          // repeat
        } else if (cfg.repeatsNum >= 2) {
            restIdx = n++;
            n++;                                   // repeat
            finalRunIdx = n++;
        }
        const int coolDownIdx = cfg.coolDown ? n++ : -1;
        switch (phase) {
        case WARM_UP:   return warmUpIdx;
        case COOL_DOWN: return coolDownIdx;
        case REST:      return restIdx;
        case RUN:
            return (!cfg.lastRest && cfg.repeatsNum >= 2 && repeat >= cfg.repeatsNum) ? finalRunIdx
                                                                                      : runIdx;
        }
        return -1;
    }
    // How the current phase ends: "open", "t<seconds>" or "d<metres>".
    std::string end() const
    {
        const auto of = [](Metric m, uint32_t t, float d) {
            if (m == TIME && t > 0) return "t" + std::to_string(t);
            if (m == DISTANCE && d > 0.0f) return "d" + std::to_string(static_cast<int>(d));
            return std::string("open");
        };
        switch (phase) {
        case RUN:  return of(cfg.runMetric, cfg.runTime, cfg.runDistance);
        case REST: return of(cfg.restMetric, cfg.restTime, cfg.restDistance);
        default:   return "open";
        }
    }
};

IntervalsSpec::Phase specPhase(RunningIntervals::Metric m, uint32_t sec, float metres)
{
    IntervalsSpec::Phase p;
    p.end = m == RunningIntervals::TIME ? IntervalsSpec::Phase::End::Time
          : m == RunningIntervals::DISTANCE ? IntervalsSpec::Phase::End::Distance
          : IntervalsSpec::Phase::End::Open;
    p.timeMs = sec * 1000u;
    p.distanceCm = static_cast<uint32_t>(metres * 100.0f);
    return p;
}

std::string phaseName(int phase)
{
    static const char* kNames[] = {"WARM_UP", "RUN", "REST", "COOL_DOWN"};
    return kNames[phase];
}

int phaseOf(Intensity in)
{
    switch (in) {
    case Intensity::Warmup:   return RunningIntervals::WARM_UP;
    case Intensity::Rest:     return RunningIntervals::REST;
    case Intensity::Cooldown: return RunningIntervals::COOL_DOWN;
    default:                  return RunningIntervals::RUN;
    }
}

// Phase sequence as "PHASE#repeat/total:end@fitIndex" entries, ending in
// "done" or cut at @p limit.
std::vector<std::string> runningSequence(RunningIntervals::Cfg cfg, size_t limit)
{
    RunningIntervals o{cfg};
    o.start();
    std::vector<std::string> out;
    while (!o.completed && out.size() < limit) {
        out.push_back(phaseName(o.phase) + "#" + std::to_string(o.repeat) + "/" +
                      std::to_string(cfg.repeatsNum) + ":" + o.end() + "@" +
                      std::to_string(o.wktStepIndex()));
        o.advance();
    }
    if (o.completed) out.push_back("done");
    return out;
}

// The same from buildIntervals() + Engine. @p manualEvery: end every step by
// hand (R2), else only the open ones.
std::vector<std::string> engineSequence(RunningIntervals::Cfg cfg, size_t limit, bool manualEvery)
{
    IntervalsSpec spec;
    spec.repeats  = cfg.repeatsNum;
    spec.run      = specPhase(cfg.runMetric, cfg.runTime, cfg.runDistance);
    spec.rest     = specPhase(cfg.restMetric, cfg.restTime, cfg.restDistance);
    spec.warmUp   = cfg.warmUp;
    spec.coolDown = cfg.coolDown;
    spec.lastRest = cfg.lastRest;
    auto prog = std::make_unique<Program>();
    const IntervalsLayout layout = buildIntervals(spec, *prog);

    Engine e;
    uint32_t t = 0, d = 0;
    e.start(*prog, t, d);
    std::vector<std::string> out;
    while (e.status().running && out.size() < limit) {
        const Step& s = prog->steps[e.status().step];
        const int phase = phaseOf(s.intensity);
        std::string end = "open";
        if (s.end == StepEnd::Time) end = "t" + std::to_string(s.durationMs / 1000);
        if (s.end == StepEnd::Distance) end = "d" + std::to_string(s.distanceCm / 100);
        uint16_t fitIndex = 0xFFFF;
        const int fit = fitStepIndex(*prog, e.status().step, fitIndex) ? fitIndex : -2;
        out.push_back(phaseName(phase) + "#" +
                      std::to_string(intervalsRepeat(spec, layout, e.status())) + "/" +
                      std::to_string(spec.repeats) + ":" + end + "@" + std::to_string(fit));
        if (manualEvery || s.end == StepEnd::Open) {
            e.next(t, d);
        } else if (s.end == StepEnd::Time) {
            t += s.durationMs;
            e.update(t, d);
        } else {
            d += s.distanceCm;
            e.update(t, d);
        }
    }
    if (e.status().finished) out.push_back("done");
    return out;
}

}  // namespace

TEST(WorkoutEngine, IntervalsMatchTheRunningAppStateMachine)
{
    using M = RunningIntervals::Metric;
    const struct { M m; uint32_t t; float d; } kEnds[] = {
        {M::OPEN, 0, 0.0f}, {M::TIME, 90, 0.0f}, {M::DISTANCE, 0, 400.0f},
        {M::TIME, 0, 0.0f},        // time selected but zero: open
        {M::DISTANCE, 0, 0.0f},    // distance selected but zero: open
    };
    const uint8_t kRepeats[] = {0, 1, 2, 3, 5, 20};
    int cases = 0;
    for (uint8_t reps : kRepeats)
    for (const auto& run : kEnds)
    for (const auto& rest : kEnds)
    for (int flags = 0; flags < 8; ++flags) {
        RunningIntervals::Cfg cfg{reps, run.m, run.t, run.d, rest.m, rest.t, rest.d,
                              (flags & 1) != 0, (flags & 2) != 0, (flags & 4) != 0};
        const size_t limit = 60;  // covers 20 repeats; cuts the unlimited ones
        const auto want = runningSequence(cfg, limit);
        SCOPED_TRACE("repeats " + std::to_string(reps) + " flags " + std::to_string(flags) +
                     " run " + std::to_string(int(run.m)) + " rest " + std::to_string(int(rest.m)));
        EXPECT_EQ(engineSequence(cfg, limit, false), want);
        EXPECT_EQ(engineSequence(cfg, limit, true), want);  // R2 on every phase: same order
        ++cases;
    }
    EXPECT_EQ(cases, 6 * 5 * 5 * 8);
}

TEST(WorkoutEngine, IntervalsLayoutMatchesTheRunningAppFitSteps)
{
    // The Running app's recording (Service::emitIntervalsWorkout) writes these steps.
    auto prog = std::make_unique<Program>();
    IntervalsSpec spec;
    spec.repeats = 4;
    spec.run.end = IntervalsSpec::Phase::End::Distance;
    spec.run.distanceCm = 40000;
    spec.rest.end = IntervalsSpec::Phase::End::Time;
    spec.rest.timeMs = 60000;
    spec.lastRest = false;
    const IntervalsLayout l = buildIntervals(spec, *prog);
    ASSERT_EQ(prog->stepCount, 6);  // warm-up, run, rest, repeat x3, final run, cool-down
    EXPECT_EQ(l.warmUp, 0);
    EXPECT_EQ(l.run, 1);
    EXPECT_EQ(l.rest, 2);
    EXPECT_EQ(l.repeat, 3);
    EXPECT_EQ(prog->steps[3].repeatFrom, 1);
    EXPECT_EQ(prog->steps[3].repeatCount, 3u);
    EXPECT_EQ(prog->steps[3].raw.durationType, 6);
    EXPECT_EQ(prog->steps[3].raw.targetValue, 3u);
    EXPECT_EQ(l.finalRun, 4);
    EXPECT_EQ(l.coolDown, 5);
    EXPECT_EQ(prog->steps[1].raw.durationType, 1);      // distance
    EXPECT_EQ(prog->steps[1].raw.durationValue, 40000u);
    EXPECT_EQ(prog->steps[2].raw.durationValue, 60000u);
    EXPECT_EQ(prog->steps[0].raw.intensity, 2);         // warmup
    EXPECT_STREQ(prog->name, "Intervals");
}

// --- Edge cases --------------------------------------------------------------------

TEST(WorkoutEngine, NestedBlocksSharingAFirstStep)
{
    // [A B] x2, the whole of it x3: 12 steps, labels follow the inner block.
    P prog;
    prog.time(1000).time(1000).repeat(0, 2).repeat(0, 3);
    const Walk r = runAll(*prog);
    EXPECT_EQ(r.steps, (std::vector<uint16_t>{0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}));
    EXPECT_EQ(r.reps[0], "1/2");
    EXPECT_EQ(r.reps[2], "2/2");
    EXPECT_EQ(r.reps[4], "1/2");  // the inner block starts again on the outer pass
}

TEST(WorkoutEngine, RepeatCountZeroRunsOnce)
{
    P prog;
    prog.time(1000).repeat(0, 0).time(1000);
    const Walk r = runAll(*prog);
    EXPECT_EQ(r.steps, (std::vector<uint16_t>{0, 2}));
    EXPECT_EQ(r.reps[0], "1/1");  // not "1/0", which would read as unlimited
}

TEST(WorkoutEngine, LastStepEndingByItselfReportsTheLap)
{
    P prog;
    prog.time(1000).dist(5000);
    Engine e;
    e.start(*prog, 0, 0);
    e.update(1000, 0);
    e.update(3000, 4999, 4000, true);
    EXPECT_TRUE(e.status().leadIn);
    EXPECT_EQ(e.update(4000, 5200, 4000, true), Change::Auto);
    EXPECT_TRUE(e.status().finished);
    EXPECT_EQ(e.ended().step, 1);
    EXPECT_EQ(e.ended().how, Change::Auto);
    EXPECT_EQ(e.ended().distanceCm, 5200u);
    // Nothing of the finished step is left showing.
    EXPECT_FALSE(e.status().leadIn);
    EXPECT_EQ(e.status().remainingCm, 0u);
    EXPECT_EQ(e.status().stepDistanceCm, 0u);
}

TEST(WorkoutEngine, StopClearsTheLeadIn)
{
    P prog;
    prog.time(10000);
    Engine e;
    e.start(*prog, 0, 0);
    e.update(8000, 0);
    ASSERT_TRUE(e.status().leadIn);
    e.stop();
    EXPECT_FALSE(e.status().leadIn);
}

TEST(WorkoutEngine, OneEngineRunsProgramsInTurn)
{
    const P first = fourHundreds();
    P second;
    second.time(1000).repeat(0, 2);
    Engine e;
    e.start(*first, 0, 0);
    e.next(0, 0);
    e.next(0, 0);
    e.next(0, 0);
    e.start(*second, 500, 500);  // counters from the first must not leak in
    EXPECT_EQ(e.status().step, 0);
    EXPECT_EQ(e.status().rep, 1u);
    EXPECT_EQ(e.status().reps, 2u);
    EXPECT_EQ(e.update(1500, 500), Change::Auto);
    EXPECT_EQ(e.status().rep, 2u);
}

TEST(WorkoutEngine, TotalsBackwardsDoNotEndAStep)
{
    P prog;
    prog.time(1000).time(1000);
    Engine e;
    e.start(*prog, 5000, 5000);
    EXPECT_EQ(e.update(4000, 4000), Change::None);  // behind the step's start
    EXPECT_EQ(e.status().stepTimeMs, 0u);
    EXPECT_EQ(e.status().step, 0);
}

TEST(WorkoutEngine, TotalsDoNotOverflowOnDeepNesting)
{
    // One step inside four blocks of 99: 96 million runs of it.
    P prog;
    prog.time(1000).repeat(0, 99).repeat(0, 99).repeat(0, 99).repeat(0, 99);
    const Totals t = totals(*prog);
    EXPECT_EQ(t.stepsRun, 99ull * 99 * 99 * 99);
    EXPECT_EQ(t.timeMs, 99ull * 99 * 99 * 99 * 1000);
}

TEST(WorkoutEngine, FitStepIndexSkipsForeverRepeats)
{
    auto prog = std::make_unique<Program>();
    IntervalsSpec spec;  // repeats 0: warm-up, run, rest, repeat-forever, cool-down
    const IntervalsLayout l = buildIntervals(spec, *prog);
    ASSERT_EQ(prog->stepCount, 5);
    uint16_t i = 0;
    ASSERT_TRUE(fitStepIndex(*prog, l.rest, i));
    EXPECT_EQ(i, 2);
    EXPECT_FALSE(fitStepIndex(*prog, l.repeat, i));  // not written
    ASSERT_TRUE(fitStepIndex(*prog, l.coolDown, i));
    EXPECT_EQ(i, 3);                                // where the file has it
    EXPECT_FALSE(fitStepIndex(*prog, 5, i));        // out of range

    // A program without forever repeats keeps its indexes.
    const P plain = fourHundreds();
    ASSERT_TRUE(fitStepIndex(*plain, 7, i));
    EXPECT_EQ(i, 7);
}

TEST(WorkoutEngine, IntervalsClampRepeats)
{
    auto prog = std::make_unique<Program>();
    IntervalsSpec spec;
    spec.repeats = 200;
    const IntervalsLayout l = buildIntervals(spec, *prog);
    EXPECT_EQ(prog->steps[l.repeat].repeatCount, kMaxRepeatCount);
}
