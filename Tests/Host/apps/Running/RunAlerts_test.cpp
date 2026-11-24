// Settings > Run Alerts: the Run/Walk start, the segment rules, and the pace
// and heart-rate range alerts (RunAlerts.hpp).

#include <gtest/gtest.h>

#include <vector>

#include "RunAlerts.hpp"

namespace {

using RunAlerts::Kind;
using RunAlerts::Monitor;
using RunAlerts::Segment;
using RunAlerts::StartMode;
using Track::IntervalsPhase;
using SDK::Workout::Intensity;

constexpr int16_t kNoWorkout = RunAlerts::kNoWorkout;

/// Speed for a pace in s/km.
float mps(float secPerKm)
{
    return 1000.0f / secPerKm;
}

/// One alert as heard: the second it played, what, and its value.
struct Heard {
    uint32_t sec;
    Kind     kind;
    float    value;
};

/**
 * A run, one active second per step(). Distance follows the speed while there
 * is a GPS reading. Quiet comes from a BuzzClock, as in the Service: every
 * alert but the back-in tap starts the buzzer.
 */
class SimRun {
public:
    explicit SimRun(const Settings::RunAlerts& cfg, Segment first = RunAlerts::kFreeRun)
    {
        monitor.configure(cfg);
        monitor.startSegment(first, 0);
    }

    /// One second. @p quietFromOutside: something else is sounding.
    void step(float speedMps, bool gps = true, float bpm = 0.0f, bool hr = false,
              bool quietFromOutside = false)
    {
        ++sec;
        if (gps) {
            distanceCm += speedMps * 100.0f;
        }
        RunAlerts::Input in;
        in.speedMps   = gps ? speedMps : 0.0f;
        in.speedValid = gps;
        in.distanceCm = static_cast<uint32_t>(distanceCm);
        in.hrBpm      = bpm;
        in.hrValid    = hr;
        const uint32_t nowMs = sec * 1000u;
        const bool quiet = clock.quiet(nowMs) || quietFromOutside;
        const RunAlerts::Alert a = monitor.tick(in, quiet);
        if (a.kind != Kind::None) {
            heard.push_back({sec, a.kind, a.value});
            if (a.kind != Kind::BackIn) {
                clock.started(nowMs);
            }
        }
    }

    /// @p n seconds of pace @p secPerKm with GPS.
    void pace(uint32_t n, float secPerKm)
    {
        for (uint32_t i = 0; i < n; ++i) {
            step(mps(secPerKm));
        }
    }

    /// @p n seconds of heart rate @p bpm (trusted), at an in-range pace.
    void heart(uint32_t n, float bpm, bool trusted = true)
    {
        for (uint32_t i = 0; i < n; ++i) {
            step(mps(300.0f), true, bpm, trusted);
        }
    }

    void startSegment(Segment s)
    {
        monitor.startSegment(s, static_cast<uint32_t>(distanceCm));
    }

    std::vector<Kind> kinds() const
    {
        std::vector<Kind> out;
        for (const Heard& h : heard) {
            out.push_back(h.kind);
        }
        return out;
    }

    Monitor               monitor;
    RunAlerts::BuzzClock  clock;
    uint32_t              sec = 0;
    float                 distanceCm = 0.0f;
    std::vector<Heard>    heard;
};

Settings::RunAlerts paceRange(float fast = 270.0f, float slow = 330.0f)
{
    Settings::RunAlerts cfg;
    cfg.paceFastSecPerKm = fast;
    cfg.paceSlowSecPerKm = slow;
    return cfg;
}

Settings::RunAlerts hrRange(uint8_t low, uint8_t high)
{
    Settings::RunAlerts cfg;
    cfg.hrLowBpm  = low;
    cfg.hrHighBpm = high;
    return cfg;
}

// =============================================================================
// Run/Walk start
// =============================================================================

TEST(RunAlertsStart, PlainStartIsFreeWithRunWalkOff)
{
    Settings::RunAlerts cfg;
    EXPECT_EQ(RunAlerts::startMode(false, kNoWorkout, cfg), StartMode::Free);
}

TEST(RunAlertsStart, PlainStartIsRunWalkWithRunWalkOn)
{
    Settings::RunAlerts cfg;
    cfg.runWalkEn = true;
    EXPECT_EQ(RunAlerts::startMode(false, kNoWorkout, cfg), StartMode::RunWalk);
}

TEST(RunAlertsStart, IntervalsFromItsMenuWinsOverRunWalk)
{
    Settings::RunAlerts cfg;
    EXPECT_EQ(RunAlerts::startMode(true, kNoWorkout, cfg), StartMode::Intervals);
    cfg.runWalkEn = true;
    EXPECT_EQ(RunAlerts::startMode(true, kNoWorkout, cfg), StartMode::Intervals);
}

TEST(RunAlertsStart, WorkoutWinsOverIntervalsAndRunWalk)
{
    Settings::RunAlerts cfg;
    cfg.runWalkEn = true;
    EXPECT_EQ(RunAlerts::startMode(false, 0, cfg), StartMode::Workout);
    EXPECT_EQ(RunAlerts::startMode(true, 3, cfg), StartMode::Workout);
    cfg.runWalkEn = false;
    EXPECT_EQ(RunAlerts::startMode(true, 3, cfg), StartMode::Workout);
}

TEST(RunAlertsStart, RunWalkSpecFields)
{
    Settings::RunAlerts cfg;
    cfg.runWalkEn      = true;
    cfg.runWalkRunSec  = 840;
    cfg.runWalkWalkSec = 120;

    using End = SDK::Workout::IntervalsSpec::Phase::End;
    const SDK::Workout::IntervalsSpec spec = RunAlerts::runWalkSpec(cfg);
    EXPECT_EQ(spec.repeats, 0);
    EXPECT_EQ(spec.run.end, End::Time);
    EXPECT_EQ(spec.run.timeMs, 840000u);
    EXPECT_EQ(spec.rest.end, End::Time);
    EXPECT_EQ(spec.rest.timeMs, 120000u);
    EXPECT_FALSE(spec.warmUp);
    EXPECT_FALSE(spec.coolDown);
    EXPECT_TRUE(spec.lastRest);
}

TEST(RunAlertsStart, RunWalkBuildsOpenRunWalkRepeats)
{
    Settings::RunAlerts cfg;  // defaults: run 4:00, walk 1:00
    SDK::Workout::Program program;
    const SDK::Workout::IntervalsLayout l =
        SDK::Workout::buildIntervals(RunAlerts::runWalkSpec(cfg), program);

    using SDK::Workout::IntervalsLayout;
    EXPECT_EQ(l.warmUp, IntervalsLayout::kNone);
    EXPECT_EQ(l.coolDown, IntervalsLayout::kNone);
    EXPECT_EQ(l.finalRun, IntervalsLayout::kNone);
    ASSERT_NE(l.run, IntervalsLayout::kNone);
    ASSERT_NE(l.rest, IntervalsLayout::kNone);
    ASSERT_NE(l.repeat, IntervalsLayout::kNone);

    EXPECT_EQ(l.run, 0);  // the run comes first: no warm-up
    EXPECT_EQ(program.steps[l.run].end, SDK::Workout::StepEnd::Time);
    EXPECT_EQ(program.steps[l.run].durationMs, 240000u);
    EXPECT_EQ(program.steps[l.run].intensity, Intensity::Active);
    EXPECT_EQ(program.steps[l.rest].end, SDK::Workout::StepEnd::Time);
    EXPECT_EQ(program.steps[l.rest].durationMs, 60000u);
    EXPECT_EQ(program.steps[l.rest].intensity, Intensity::Rest);
    EXPECT_EQ(program.steps[l.repeat].repeatCount, SDK::Workout::kRepeatForever);
}

// =============================================================================
// Segment rules
// =============================================================================

void expectSegment(Segment s, bool pace, bool hr)
{
    EXPECT_EQ(s.pace, pace);
    EXPECT_EQ(s.hr, hr);
}

TEST(RunAlertsSegments, FreeRunHasBoth)
{
    expectSegment(RunAlerts::kFreeRun, true, true);
}

TEST(RunAlertsSegments, IntervalsPhases)
{
    expectSegment(RunAlerts::forPhase(IntervalsPhase::RUN), true, true);
    expectSegment(RunAlerts::forPhase(IntervalsPhase::WARM_UP), false, true);
    expectSegment(RunAlerts::forPhase(IntervalsPhase::REST), false, true);
    expectSegment(RunAlerts::forPhase(IntervalsPhase::COOL_DOWN), false, true);
}

TEST(RunAlertsSegments, WorkoutSteps)
{
    // Active, no target: both.
    expectSegment(RunAlerts::forStep(Intensity::Active, false, false), true, true);
    // Its own speed target: pace off.
    expectSegment(RunAlerts::forStep(Intensity::Active, true, false), false, true);
    // Its own heart-rate target: HR off.
    expectSegment(RunAlerts::forStep(Intensity::Active, false, true), true, false);
    // Rest, warm-up and cool-down: pace off, HR unless targeted.
    expectSegment(RunAlerts::forStep(Intensity::Rest, false, false), false, true);
    expectSegment(RunAlerts::forStep(Intensity::Warmup, false, false), false, true);
    expectSegment(RunAlerts::forStep(Intensity::Cooldown, false, false), false, true);
    expectSegment(RunAlerts::forStep(Intensity::Rest, false, true), false, false);
}

// =============================================================================
// Defaults: no change to a run
// =============================================================================

TEST(RunAlertsMonitor, DefaultSettingsNeverAlert)
{
    Settings::RunAlerts cfg;  // all Off
    SimRun run(cfg);
    EXPECT_FALSE(run.monitor.paceOn());
    EXPECT_FALSE(run.monitor.hrOn());
    for (int i = 0; i < 300; ++i) run.step(mps(600.0f), true, 200.0f, true);
    run.startSegment(RunAlerts::forPhase(IntervalsPhase::REST));
    for (int i = 0; i < 300; ++i) run.step(mps(150.0f), true, 50.0f, true);
    run.monitor.resume();
    for (int i = 0; i < 300; ++i) run.step(mps(900.0f), true, 230.0f, true);
    EXPECT_TRUE(run.heard.empty());
}

// =============================================================================
// Pace
// =============================================================================

TEST(RunAlertsPace, OffNeverAlerts)
{
    Settings::RunAlerts cfg = hrRange(0, 0);
    SimRun run(cfg);
    run.pace(600, 600.0f);   // 10:00 /km for 10 min
    run.pace(600, 150.0f);   // 2:30 /km for 10 min
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsPace, TooSlowAlertsOnceAtSecond31WithSmoothedPace)
{
    SimRun run(paceRange());
    run.pace(31, 400.0f);
    ASSERT_EQ(run.heard.size(), 1u);
    EXPECT_EQ(run.heard[0].sec, 31u);
    EXPECT_EQ(run.heard[0].kind, Kind::PaceSlow);
    EXPECT_NEAR(run.heard[0].value, 0.400f, 0.001f);  // 6:40 /km in s/m
}

TEST(RunAlertsPace, NothingDuringTheHold)
{
    SimRun run(paceRange());
    run.pace(30, 400.0f);
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsPace, NoRepeatOverTenMinutes)
{
    SimRun run(paceRange());
    run.pace(31 + 600, 400.0f);
    EXPECT_EQ(run.kinds(), std::vector<Kind>{Kind::PaceSlow});
}

TEST(RunAlertsPace, BackInIsSilentByDefault)
{
    SimRun run(paceRange());
    run.pace(60, 400.0f);
    run.pace(180, 300.0f);
    EXPECT_EQ(run.kinds(), std::vector<Kind>{Kind::PaceSlow});
}

TEST(RunAlertsPace, BackInIsATapWhenEnabled)
{
    Settings::RunAlerts cfg = paceRange();
    cfg.backInTap = true;
    SimRun run(cfg);
    run.pace(60, 400.0f);
    run.pace(180, 300.0f);
    EXPECT_EQ(run.kinds(), (std::vector<Kind>{Kind::PaceSlow, Kind::BackIn}));
}

TEST(RunAlertsPace, OutAgainAlertsAgain)
{
    SimRun run(paceRange());
    run.pace(60, 400.0f);
    run.pace(180, 300.0f);
    run.pace(180, 400.0f);
    EXPECT_EQ(run.kinds(), (std::vector<Kind>{Kind::PaceSlow, Kind::PaceSlow}));
}

TEST(RunAlertsPace, TooFastHasItsOwnKind)
{
    SimRun run(paceRange());
    run.pace(31, 240.0f);
    ASSERT_EQ(run.heard.size(), 1u);
    EXPECT_EQ(run.heard[0].sec, 31u);
    EXPECT_EQ(run.heard[0].kind, Kind::PaceFast);
    EXPECT_NEAR(run.heard[0].value, 0.240f, 0.001f);
}

TEST(RunAlertsPace, SlowThenFastAlertsFast)
{
    SimRun run(paceRange());
    run.pace(60, 400.0f);
    run.pace(300, 200.0f);
    EXPECT_EQ(run.kinds(), (std::vector<Kind>{Kind::PaceSlow, Kind::PaceFast}));
}

TEST(RunAlertsPace, OnlyOneLimitSet)
{
    // Too Slow only: running fast never alerts; slow does.
    SimRun fast(paceRange(0.0f, 330.0f));
    fast.pace(600, 150.0f);
    EXPECT_TRUE(fast.heard.empty());
    SimRun slow(paceRange(0.0f, 330.0f));
    slow.pace(60, 400.0f);
    EXPECT_EQ(slow.kinds(), std::vector<Kind>{Kind::PaceSlow});

    // Too Fast only: standing still never alerts; fast does.
    SimRun still(paceRange(270.0f, 0.0f));
    still.pace(600, 1200.0f);
    EXPECT_TRUE(still.heard.empty());
    SimRun quick(paceRange(270.0f, 0.0f));
    quick.pace(60, 240.0f);
    EXPECT_EQ(quick.kinds(), std::vector<Kind>{Kind::PaceFast});
}

TEST(RunAlertsPace, HoldCountsOnlySecondsWithGps)
{
    SimRun run(paceRange());
    for (int i = 0; i < 10; ++i) run.step(mps(400.0f), false);
    run.pace(31, 400.0f);
    ASSERT_EQ(run.heard.size(), 1u);
    EXPECT_EQ(run.heard[0].sec, 41u);
}

TEST(RunAlertsPace, NoGpsNoAlert)
{
    SimRun run(paceRange());
    for (int i = 0; i < 600; ++i) run.step(mps(400.0f), false);
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsPace, ResumeHoldsAgain)
{
    SimRun run(paceRange());
    run.pace(60, 300.0f);   // in range
    run.monitor.resume();   // a pause: no seconds pass
    run.pace(30, 400.0f);
    EXPECT_TRUE(run.heard.empty());
    run.pace(1, 400.0f);
    ASSERT_EQ(run.heard.size(), 1u);
    EXPECT_EQ(run.heard[0].sec, 91u);
    EXPECT_EQ(run.heard[0].kind, Kind::PaceSlow);
}

TEST(RunAlertsPace, ResumeDoesNotRepeat)
{
    SimRun run(paceRange());
    run.pace(60, 400.0f);
    run.monitor.resume();
    run.pace(600, 400.0f);
    run.monitor.resume();
    run.pace(600, 400.0f);
    EXPECT_EQ(run.kinds(), std::vector<Kind>{Kind::PaceSlow});
}

TEST(RunAlertsPace, JitterWithinTheDeadbandNeverAlerts)
{
    // 2 s/km either side of the Too Slow edge, inside the 3 s/km deadband.
    SimRun run(paceRange());
    run.pace(60, 328.0f);
    for (int i = 0; i < 600; ++i) {
        run.pace(1, (i % 2) ? 332.0f : 328.0f);
    }
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsPace, WalkSegmentSilencesPaceNotHr)
{
    Settings::RunAlerts cfg = paceRange();
    cfg.hrHighBpm = 150;
    SimRun run(cfg, RunAlerts::forPhase(IntervalsPhase::REST));
    EXPECT_FALSE(run.monitor.paceOn());
    EXPECT_TRUE(run.monitor.hrOn());
    for (int i = 0; i < 300; ++i) run.step(mps(700.0f), true, 170.0f, true);
    EXPECT_EQ(run.kinds(), std::vector<Kind>{Kind::HrHigh});
}

TEST(RunAlertsPace, SpeedTargetStepSilencesPaceHrTargetStepSilencesHr)
{
    Settings::RunAlerts cfg = paceRange();
    cfg.hrHighBpm = 150;

    SimRun speedStep(cfg, RunAlerts::forStep(Intensity::Active, true, false));
    for (int i = 0; i < 300; ++i) speedStep.step(mps(400.0f), true, 170.0f, true);
    EXPECT_EQ(speedStep.kinds(), std::vector<Kind>{Kind::HrHigh});

    SimRun hrStep(cfg, RunAlerts::forStep(Intensity::Active, false, true));
    for (int i = 0; i < 300; ++i) hrStep.step(mps(400.0f), true, 170.0f, true);
    EXPECT_EQ(hrStep.kinds(), std::vector<Kind>{Kind::PaceSlow});
}

TEST(RunAlertsPace, NewRunPhaseForgetsTheOldAlert)
{
    SimRun run(paceRange(), RunAlerts::forPhase(IntervalsPhase::RUN));
    run.pace(120, 400.0f);                                    // alert at 31
    run.startSegment(RunAlerts::forPhase(IntervalsPhase::REST));
    run.pace(60, 700.0f);                                     // walking: nothing
    run.startSegment(RunAlerts::forPhase(IntervalsPhase::RUN));
    run.pace(120, 400.0f);                                    // alert again
    ASSERT_EQ(run.kinds(), (std::vector<Kind>{Kind::PaceSlow, Kind::PaceSlow}));
    EXPECT_EQ(run.heard[0].sec, 31u);
    EXPECT_EQ(run.heard[1].sec, 180u + 31u);
}

// =============================================================================
// Heart rate
// =============================================================================

TEST(RunAlertsHr, HighOnceNoRepeatBackInOutAgain)
{
    SimRun run(hrRange(0, 160));
    run.heart(300, 170.0f);
    ASSERT_EQ(run.kinds(), std::vector<Kind>{Kind::HrHigh});
    EXPECT_EQ(run.heard[0].sec, 31u);
    EXPECT_FLOAT_EQ(run.heard[0].value, 170.0f);
    run.heart(120, 150.0f);   // back in: silent
    EXPECT_EQ(run.heard.size(), 1u);
    run.heart(120, 170.0f);   // out again
    EXPECT_EQ(run.kinds(), (std::vector<Kind>{Kind::HrHigh, Kind::HrHigh}));
}

TEST(RunAlertsHr, LowArmedOnlyAfterReachingTheRange)
{
    SimRun run(hrRange(120, 0));
    run.heart(300, 100.0f);   // warming up below Low: not armed
    EXPECT_TRUE(run.heard.empty());
    run.heart(60, 130.0f);    // reaches the range: armed
    EXPECT_TRUE(run.heard.empty());
    run.heart(60, 100.0f);
    EXPECT_EQ(run.kinds(), std::vector<Kind>{Kind::HrLow});
}

TEST(RunAlertsHr, LowReArmedAfterResume)
{
    SimRun run(hrRange(120, 0));
    run.heart(60, 130.0f);
    run.heart(60, 100.0f);    // HR low
    run.heart(60, 130.0f);    // back in, silent
    ASSERT_EQ(run.kinds(), std::vector<Kind>{Kind::HrLow});
    run.monitor.resume();
    run.heart(300, 100.0f);   // disarmed after the resume
    EXPECT_EQ(run.heard.size(), 1u);
    run.heart(60, 130.0f);    // armed again
    run.heart(60, 100.0f);
    EXPECT_EQ(run.kinds(), (std::vector<Kind>{Kind::HrLow, Kind::HrLow}));
}

TEST(RunAlertsHr, UntrustedHrNeverAlerts)
{
    SimRun run(hrRange(120, 160));
    run.heart(600, 200.0f, false);
    run.heart(600, 60.0f, false);
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsHr, HrCarriesOnAcrossRunWalkPhasesWithNoNewHold)
{
    SimRun run(hrRange(0, 160), RunAlerts::forPhase(IntervalsPhase::RUN));
    run.heart(60, 140.0f);
    run.startSegment(RunAlerts::forPhase(IntervalsPhase::REST));
    run.heart(30, 170.0f);
    // Out past the deadband and dwell, a few seconds in: not after a new
    // 30 s hold, which would put it at 91 or later.
    ASSERT_EQ(run.kinds(), std::vector<Kind>{Kind::HrHigh});
    EXPECT_LE(run.heard[0].sec, 66u);
}

// =============================================================================
// Never over another alert
// =============================================================================

TEST(RunAlertsArbitration, PaceAndHrInTheSameSecondPlayApart)
{
    Settings::RunAlerts cfg = paceRange();
    cfg.hrHighBpm = 160;
    SimRun run(cfg);
    for (int i = 0; i < 60; ++i) run.step(mps(400.0f), true, 170.0f, true);
    ASSERT_EQ(run.kinds(), (std::vector<Kind>{Kind::PaceSlow, Kind::HrHigh}));
    EXPECT_EQ(run.heard[0].sec, 31u);
    // At least kQuietMs apart, on the next whole second.
    EXPECT_EQ(run.heard[1].sec, 34u);
}

TEST(RunAlertsArbitration, QuietHoldsAnAlert)
{
    SimRun run(paceRange());
    run.pace(30, 400.0f);
    for (int i = 0; i < 10; ++i) run.step(mps(400.0f), true, 0.0f, false, true);
    EXPECT_TRUE(run.heard.empty());
    run.pace(1, 400.0f);
    ASSERT_EQ(run.heard.size(), 1u);
    EXPECT_EQ(run.heard[0].sec, 41u);
    EXPECT_EQ(run.heard[0].kind, Kind::PaceSlow);
}

TEST(RunAlertsArbitration, PendingAlertDroppedWhenBackInFirst)
{
    // Too slow is due at 31 but something else is sounding; the runner is
    // back in range before it ends, so nothing plays, not even the tap.
    Settings::RunAlerts cfg = paceRange();
    cfg.backInTap = true;
    SimRun run(cfg);
    run.pace(30, 400.0f);
    for (int i = 0; i < 150; ++i) run.step(mps(290.0f), true, 0.0f, false, true);
    run.pace(300, 290.0f);
    EXPECT_TRUE(run.heard.empty());
}

TEST(RunAlertsArbitration, BuzzClockQuietFor2500Ms)
{
    RunAlerts::BuzzClock clock;
    EXPECT_FALSE(clock.quiet(0));
    EXPECT_FALSE(clock.quiet(100000));
    clock.started(100000);
    EXPECT_TRUE(clock.quiet(100000));
    EXPECT_TRUE(clock.quiet(102499));
    EXPECT_FALSE(clock.quiet(102500));
    // Across the millisecond counter's wrap.
    clock.started(0xFFFFFF00u);
    EXPECT_TRUE(clock.quiet(0x00000100u));
}

// =============================================================================
// What each alert does
// =============================================================================

using Sound = SDK::Workout::ZoneAlerts::Alert;

TEST(RunAlertsFeedback, BackInIsATapOnlyWithTheBacklightLeftAlone)
{
    const RunAlerts::Feedback f = RunAlerts::feedback(Kind::BackIn);
    EXPECT_TRUE(f.tap);
    EXPECT_FALSE(f.backlight);
    EXPECT_EQ(f.sound, Sound::None);
    EXPECT_FALSE(f.message);
}

TEST(RunAlertsFeedback, OutOfRangeAlertsSoundLightAndShowThePopUp)
{
    struct Row { Kind kind; Sound sound; Track::RunAlertKind msg; };
    const Row rows[] = {
        {Kind::PaceFast, Sound::Above, Track::RunAlertKind::PACE_FAST},
        {Kind::PaceSlow, Sound::Below, Track::RunAlertKind::PACE_SLOW},
        {Kind::HrHigh,   Sound::Above, Track::RunAlertKind::HR_HIGH},
        {Kind::HrLow,    Sound::Below, Track::RunAlertKind::HR_LOW},
    };
    for (const Row& r : rows) {
        const RunAlerts::Feedback f = RunAlerts::feedback(r.kind);
        EXPECT_EQ(f.sound, r.sound);
        EXPECT_FALSE(f.tap);
        EXPECT_TRUE(f.backlight);
        EXPECT_TRUE(f.message);
        EXPECT_EQ(f.kind, r.msg);
    }
}

TEST(RunAlertsFeedback, BacklightOnlyWithASound)
{
    const Kind kinds[] = {Kind::None, Kind::PaceFast, Kind::PaceSlow, Kind::HrHigh, Kind::HrLow, Kind::BackIn};
    for (Kind k : kinds) {
        const RunAlerts::Feedback f = RunAlerts::feedback(k);
        EXPECT_EQ(f.backlight, f.sound != Sound::None);
    }
    const RunAlerts::Feedback none = RunAlerts::feedback(Kind::None);
    EXPECT_FALSE(none.tap);
    EXPECT_FALSE(none.message);
}

// =============================================================================
// Settings > Run Alerts pickers
// =============================================================================

using RunAlerts::Field;

constexpr uint8_t kDefaultThresholds[6] = {95, 114, 133, 152, 171, 190};

uint32_t seed(Field f, const Settings::RunAlerts& cfg, bool imperial = false)
{
    return RunAlerts::pickerSeed(f, cfg, imperial, kDefaultThresholds, 6);
}

TEST(RunAlertsPickers, PaceRoundTripsPerKmAndPerMile)
{
    for (uint32_t v = RunAlerts::kPaceMinDisplaySec; v <= RunAlerts::kPaceMaxDisplaySec; v += RunAlerts::kStepSec) {
        EXPECT_EQ(RunAlerts::paceToDisplay(RunAlerts::paceFromDisplay(v, false), false), v);
        EXPECT_EQ(RunAlerts::paceToDisplay(RunAlerts::paceFromDisplay(v, true), true), v);
    }
    // 5:00 /mi is kept in s/km, unrounded.
    EXPECT_NEAR(RunAlerts::paceFromDisplay(300, true), 186.411f, 0.001f);
    EXPECT_EQ(RunAlerts::paceToDisplay(RunAlerts::paceFromDisplay(300, true), true), 300u);
}

TEST(RunAlertsPickers, PaceShowsInTheOtherUnitAndOffStaysOff)
{
    EXPECT_EQ(RunAlerts::paceToDisplay(300.0f, true), 483u);   // 5:00 /km is 8:03 /mi
    EXPECT_EQ(RunAlerts::paceToDisplay(0.0f, false), 0u);
    EXPECT_EQ(RunAlerts::paceToDisplay(0.0f, true), 0u);
}

TEST(RunAlertsPickers, RunWalkTimesFrom5SecondsTo60Minutes)
{
    Settings::RunAlerts cfg;
    for (Field f : {Field::RunTime, Field::WalkTime}) {
        const RunAlerts::Bounds b = RunAlerts::pickerBounds(f, cfg, false);
        EXPECT_EQ(b.min, 5u);      // 0:00 can't be picked
        EXPECT_EQ(b.max, 3600u);
    }
}

TEST(RunAlertsPickers, PaceLimitsAreNarrowedByTheOtherOne)
{
    Settings::RunAlerts cfg;
    RunAlerts::Bounds b = RunAlerts::pickerBounds(Field::PaceFast, cfg, false);
    EXPECT_EQ(b.min, 120u);
    EXPECT_EQ(b.max, 1200u);

    cfg.paceSlowSecPerKm = 360.0f;
    b = RunAlerts::pickerBounds(Field::PaceFast, cfg, false);
    EXPECT_EQ(b.max, 355u);

    cfg.paceSlowSecPerKm = 0.0f;
    cfg.paceFastSecPerKm = 270.0f;
    b = RunAlerts::pickerBounds(Field::PaceSlow, cfg, false);
    EXPECT_EQ(b.min, 275u);
    EXPECT_EQ(b.max, 1200u);

    // Off the 5 s grid (a pace picked per km, shown per mile): still strictly apart.
    cfg = Settings::RunAlerts{};
    cfg.paceSlowSecPerKm = 331.0f / RunAlerts::kKmPerMile;
    b = RunAlerts::pickerBounds(Field::PaceFast, cfg, true);
    EXPECT_EQ(b.max, 325u);
}

TEST(RunAlertsPickers, NothingToPickWhenTheOtherLimitLeavesNoRoom)
{
    Settings::RunAlerts cfg;
    cfg.paceSlowSecPerKm = 120.0f;   // Too Slow at 2:00
    EXPECT_TRUE(RunAlerts::pickerBounds(Field::PaceFast, cfg, false).empty());
    cfg = Settings::RunAlerts{};
    cfg.paceFastSecPerKm = 1200.0f;  // Too Fast at 20:00
    EXPECT_TRUE(RunAlerts::pickerBounds(Field::PaceSlow, cfg, false).empty());
}

TEST(RunAlertsPickers, HrLimitsAreNarrowedByTheOtherOne)
{
    Settings::RunAlerts cfg;
    RunAlerts::Bounds b = RunAlerts::pickerBounds(Field::HrHigh, cfg, false);
    EXPECT_EQ(b.min, 40u);
    EXPECT_EQ(b.max, 220u);
    cfg.hrLowBpm = 120;
    EXPECT_EQ(RunAlerts::pickerBounds(Field::HrHigh, cfg, false).min, 121u);
    cfg = Settings::RunAlerts{};
    cfg.hrHighBpm = 165;
    EXPECT_EQ(RunAlerts::pickerBounds(Field::HrLow, cfg, false).max, 164u);
}

TEST(RunAlertsPickers, SeedsAreTheCurrentValueOnTheStep)
{
    Settings::RunAlerts cfg;
    cfg.runWalkRunSec    = 242;
    cfg.runWalkWalkSec   = 60;
    cfg.paceFastSecPerKm = 268.0f;
    cfg.hrHighBpm        = 150;
    EXPECT_EQ(seed(Field::RunTime, cfg), 240u);
    EXPECT_EQ(seed(Field::WalkTime, cfg), 60u);
    EXPECT_EQ(seed(Field::PaceFast, cfg), 270u);
    EXPECT_EQ(seed(Field::HrHigh, cfg), 150u);
}

TEST(RunAlertsPickers, SeedsWhenOffAreSuggestions)
{
    Settings::RunAlerts cfg;
    EXPECT_EQ(seed(Field::PaceFast, cfg), 270u);   // 4:30
    EXPECT_EQ(seed(Field::PaceSlow, cfg), 360u);   // 6:00
    EXPECT_EQ(seed(Field::PaceFast, cfg, true), 270u);
    EXPECT_EQ(seed(Field::HrHigh, cfg), 171u);     // zone 5 starts
    EXPECT_EQ(seed(Field::HrLow, cfg), 133u);      // zone 3 starts
}

TEST(RunAlertsPickers, SuggestionsStayWithinTheOtherLimit)
{
    Settings::RunAlerts cfg;
    cfg.paceSlowSecPerKm = 240.0f;   // Too Slow 4:00: Too Fast opens at 3:55
    EXPECT_EQ(seed(Field::PaceFast, cfg), 235u);
    cfg = Settings::RunAlerts{};
    cfg.hrLowBpm = 180;              // Low 180: High opens at 181
    EXPECT_EQ(seed(Field::HrHigh, cfg), 181u);
    // No thresholds: the bottom of the list.
    EXPECT_EQ(RunAlerts::pickerSeed(Field::HrHigh, Settings::RunAlerts{}, false, nullptr, 0), 40u);
}

TEST(RunAlertsPickers, FieldTextsAndTitles)
{
    char buf[24];
    Settings::RunAlerts cfg;   // run 4:00, walk 1:00, limits Off
    RunAlerts::fieldText(Field::RunTime, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "04:00 min");
    RunAlerts::fieldText(Field::WalkTime, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "01:00 min");
    RunAlerts::fieldText(Field::PaceFast, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "OFF");
    RunAlerts::fieldText(Field::HrLow, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "OFF");

    cfg.paceSlowSecPerKm = 300.0f;
    cfg.hrHighBpm        = 165;
    RunAlerts::fieldText(Field::PaceSlow, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "5:00 /km");
    RunAlerts::fieldText(Field::PaceSlow, cfg, true, buf, sizeof(buf));
    EXPECT_STREQ(buf, "8:03 /mi");
    RunAlerts::fieldText(Field::HrHigh, cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "165 bpm");

    EXPECT_EQ(RunAlerts::fieldValue(Field::RunTime, cfg, false), 240u);
    EXPECT_EQ(RunAlerts::fieldValue(Field::PaceSlow, cfg, true), 483u);
    EXPECT_EQ(RunAlerts::fieldValue(Field::PaceFast, cfg, false), 0u);
    EXPECT_EQ(RunAlerts::fieldValue(Field::HrHigh, cfg, false), 165u);

    EXPECT_STREQ(RunAlerts::fieldTitle(Field::RunTime), "RUN TIME");
    EXPECT_STREQ(RunAlerts::fieldTitle(Field::WalkTime), "WALK TIME");
    EXPECT_STREQ(RunAlerts::fieldTitle(Field::PaceFast), "TOO FAST");
    EXPECT_STREQ(RunAlerts::fieldTitle(Field::PaceSlow), "TOO SLOW");
    EXPECT_STREQ(RunAlerts::fieldTitle(Field::HrHigh), "HR HIGH");
    EXPECT_STREQ(RunAlerts::fieldTitle(Field::HrLow), "HR LOW");
}

TEST(RunAlertsPickers, SetFieldStoresEachSetting)
{
    Settings::RunAlerts cfg;
    RunAlerts::setField(Field::RunTime, 840, false, cfg);
    RunAlerts::setField(Field::WalkTime, 120, false, cfg);
    RunAlerts::setField(Field::PaceFast, 270, false, cfg);
    RunAlerts::setField(Field::PaceSlow, 600, true, cfg);
    RunAlerts::setField(Field::HrHigh, 165, false, cfg);
    RunAlerts::setField(Field::HrLow, 120, false, cfg);
    EXPECT_EQ(cfg.runWalkRunSec, 840u);
    EXPECT_EQ(cfg.runWalkWalkSec, 120u);
    EXPECT_EQ(cfg.paceFastSecPerKm, 270.0f);
    EXPECT_NEAR(cfg.paceSlowSecPerKm, 372.823f, 0.001f);   // 10:00 /mi
    EXPECT_EQ(cfg.hrHighBpm, 165);
    EXPECT_EQ(cfg.hrLowBpm, 120);

    // 0 turns a limit Off.
    RunAlerts::setField(Field::PaceFast, 0, false, cfg);
    RunAlerts::setField(Field::HrLow, 0, false, cfg);
    EXPECT_EQ(cfg.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(cfg.hrLowBpm, 0);
}

TEST(RunAlertsPickers, HintTexts)
{
    char buf[24];
    RunAlerts::paceText(270, buf, sizeof(buf));
    EXPECT_STREQ(buf, "4:30");
    RunAlerts::paceText(1200, buf, sizeof(buf));
    EXPECT_STREQ(buf, "20:00");
    RunAlerts::paceText(0, buf, sizeof(buf));
    EXPECT_STREQ(buf, "--:--");
    // Slower than 99:59 stays 5 characters.
    RunAlerts::paceText(5999, buf, sizeof(buf));
    EXPECT_STREQ(buf, "99:59");
    RunAlerts::paceText(20000, buf, sizeof(buf));
    EXPECT_STREQ(buf, "99:59");

    Settings::RunAlerts cfg;
    RunAlerts::paceRangeText(cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "");
    cfg.paceFastSecPerKm = 270.0f;
    RunAlerts::paceRangeText(cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Fast 4:30");
    cfg.paceSlowSecPerKm = 360.0f;
    RunAlerts::paceRangeText(cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "4:30-6:00");
    cfg.paceFastSecPerKm = 0.0f;
    RunAlerts::paceRangeText(cfg, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Slow 6:00");

    RunAlerts::hrRangeText(cfg, buf, sizeof(buf));
    EXPECT_STREQ(buf, "");
    cfg.hrHighBpm = 165;
    RunAlerts::hrRangeText(cfg, buf, sizeof(buf));
    EXPECT_STREQ(buf, "High 165");
    cfg.hrLowBpm = 120;
    RunAlerts::hrRangeText(cfg, buf, sizeof(buf));
    EXPECT_STREQ(buf, "120-165");
    cfg.hrHighBpm = 0;
    RunAlerts::hrRangeText(cfg, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Low 120");
}

// =============================================================================
// The run alert pop-up's texts
// =============================================================================

TEST(RunAlertsPopUp, TitlesAndUnits)
{
    using K = Track::RunAlertKind;
    EXPECT_STREQ(RunAlerts::alertTitle(K::PACE_FAST), "Too fast");
    EXPECT_STREQ(RunAlerts::alertTitle(K::PACE_SLOW), "Too slow");
    EXPECT_STREQ(RunAlerts::alertTitle(K::HR_HIGH), "HR high");
    EXPECT_STREQ(RunAlerts::alertTitle(K::HR_LOW), "HR low");

    EXPECT_STREQ(RunAlerts::alertUnit(K::PACE_FAST, false), "/km");
    EXPECT_STREQ(RunAlerts::alertUnit(K::PACE_SLOW, true), "/mi");
    EXPECT_STREQ(RunAlerts::alertUnit(K::HR_HIGH, false), "bpm");
    EXPECT_STREQ(RunAlerts::alertUnit(K::HR_LOW, true), "bpm");
}

TEST(RunAlertsPopUp, PaceIsShownPerKmOrMileFromSecondsPerMetre)
{
    using K = Track::RunAlertKind;
    char buf[16];
    // 0.342 s/m is 5:42 /km, and 9:10 /mi.
    RunAlerts::alertValueText(K::PACE_SLOW, 0.342f, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "5:42");
    RunAlerts::alertValueText(K::PACE_SLOW, 0.342f, true, buf, sizeof(buf));
    EXPECT_STREQ(buf, "9:10");
    RunAlerts::alertValueText(K::PACE_FAST, 0.18f, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "3:00");
    // No pace (standing) shows no value rather than 0:00.
    RunAlerts::alertValueText(K::PACE_SLOW, 0.0f, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "--:--");
}

TEST(RunAlertsPopUp, HeartRateIsWholeBpm)
{
    using K = Track::RunAlertKind;
    char buf[16];
    RunAlerts::alertValueText(K::HR_HIGH, 165.4f, false, buf, sizeof(buf));
    EXPECT_STREQ(buf, "165");
    RunAlerts::alertValueText(K::HR_LOW, 119.6f, true, buf, sizeof(buf));
    EXPECT_STREQ(buf, "120");
}

} // namespace
