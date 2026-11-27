// Oval::LapCounter, counting mode (docs/OVAL.md).
//
// Every scenario runs through Replay::run(), which checks after every reading
// that the total never decreased, and most through wholeLapsRight(), which
// checks the lap count, lap end times and the credited total at the last lap.

#include "OvalTestUtil.hpp"

#include "Oval/LapCounter.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <limits>

using namespace OvalTest;
using Oval::LapCounter;
using Oval::LaneTable;
using TraceGen::Generator;
using TraceGen::Trace;

namespace {

constexpr float kTailM = 40.0f; // run on past the last line so its crossing is seen

// Most scenarios run a few seeds, so a pass is not luck.
constexpr uint32_t kSeeds[] = {11, 23, 37, 41, 59};

bool anyEventBetween(const Replay::Result& r, size_t from, size_t to)
{
    for (size_t s : r.eventSample) {
        if (s >= from && s < to) {
            return true;
        }
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// Clean and noisy
// ---------------------------------------------------------------------------

TEST(LapCounter, CleanTenLapsLane1IsExactly4000m)
{
    Generator g(TraceGen::Track(), clean(), 1);
    g.begin(0.1f, 1);
    g.runLaps(10, steady(5.0f));
    g.run(kTailM, steady(5.0f));
    const Trace tr = g.finish();
    ASSERT_EQ(tr.crossMs.size(), 10u);

    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 500));
    ASSERT_EQ(r.events.size(), 10u);
    EXPECT_EQ(r.missed, 0);
    EXPECT_NEAR(r.events.back().totalM, 4000.0f, 0.01f);
    for (size_t k = 0; k < r.events.size(); ++k) {
        EXPECT_EQ(r.events[k].type, LapCounter::EventType::Lap);
        EXPECT_EQ(r.events[k].lapNumber, k + 1);
        EXPECT_NEAR(r.events[k].lapLengthM, 400.0f, 1e-3f);
    }
    const Replay::Match m = Replay::match(tr, r, 500);
    EXPECT_LT(worstAbs(m.lapTimeErrMs), 150.0) << "clean lap times within 0.15 s";
    // Clean GPS distance is the lane line itself, so the ratio settles at 1.
    EXPECT_NEAR(r.gpsRatio, 1.0f, 0.01f);
}

TEST(LapCounter, NoiseOf3mWithDrift)
{
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.runLaps(10, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        EXPECT_NEAR(r.events.back().totalM, 4000.0f, 0.01f);
        const Replay::Match m = Replay::match(tr, r, 3000);
        EXPECT_LT(worstAbs(m.lapTimeErrMs), 3000.0) << "seed " << seed;
    }
}

TEST(LapCounter, NoiseOf5mWithDrift)
{
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(5.0f, 0.15f), seed);
        g.begin(0.1f, 1);
        g.runLaps(10, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 4000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        EXPECT_NEAR(r.events.back().totalM, 4000.0f, 0.01f);
    }
}

// ---------------------------------------------------------------------------
// Where the run starts
// ---------------------------------------------------------------------------

TEST(LapCounter, StartingOnAStraight)
{
    const float starts[] = {0.02f, 0.10f, 0.19f, 0.55f, 0.65f}; // both straights, near both ends
    for (float start : starts) {
        Generator g(TraceGen::Track(), noisy(3.0f), 7);
        g.begin(start, 1);
        g.runLaps(6, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "start " << start;
        EXPECT_EQ(r.missed, 0) << "start " << start;
    }
}

TEST(LapCounter, StartingOnABend)
{
    const float starts[] = {0.25f, 0.35f, 0.42f, 0.78f, 0.86f, 0.95f}; // both bends: entry, apex, exit
    for (float start : starts) {
        Generator g(TraceGen::Track(), noisy(3.0f), 8);
        g.begin(start, 1);
        g.runLaps(6, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "start " << start;
        EXPECT_EQ(r.missed, 0) << "start " << start;
    }
}

TEST(LapCounter, TrackOrientationAndLatitudeDoNotMatter)
{
    const float  headings[] = {0.0f, 37.0f, 90.0f, 161.0f, 233.0f, 300.0f};
    const double lats[]     = {55.95, 0.0, -33.9, 64.1, 51.5, 40.4};
    for (int i = 0; i < 6; ++i) {
        TraceGen::Track track;
        track.headingDeg = headings[i];
        track.centreLat  = lats[i];
        track.centreLon  = -3.19 + 40.0 * i;
        Generator g(track, noisy(3.0f), 100 + i);
        g.begin(0.3f, 2);
        g.runLaps(5, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "case " << i;
    }
}

// ---------------------------------------------------------------------------
// Speeds
// ---------------------------------------------------------------------------

TEST(LapCounter, WalkerAt1_2mps)
{
    for (uint32_t seed : {3u, 4u, 5u}) {
        Generator g(TraceGen::Track(), noisy(3.0f, 0.05f), seed);
        g.begin(0.1f, 1);
        TraceGen::Speed walk = steady(1.2f);
        walk.jitter = 0.05f;
        g.runLaps(4, walk);
        g.run(kTailM, walk);
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        // A walker's crossing time is as uncertain as the noise divided by
        // the speed: several seconds with 3 m white noise.
        EXPECT_TRUE(wholeLapsRight(tr, r, 10000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
    }
}

TEST(LapCounter, SprinterAt8mps)
{
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.runLaps(8, runner(8.0f));
        g.run(kTailM, runner(8.0f));
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 2000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        for (const auto& e : r.events) {
            EXPECT_GE(e.lapDurationMs, 40000u); // 50 s laps, above the 40 s guard
        }
    }
}

// ---------------------------------------------------------------------------
// Standing on the line and jogging over it: no false laps
// ---------------------------------------------------------------------------

TEST(LapCounter, TwoMinutesStandingOnTheLineBeforeRunning)
{
    for (uint32_t seed : kSeeds) {
        // 5 m noise, and GPS distance that piles up jitter while standing, as
        // the simulator's would.
        Generator g(TraceGen::Track(), noisy(5.0f, 0.1f, 2.0f), seed);
        g.begin(0.1f, 1);
        g.stand(120.0);
        const size_t ranFrom = g.finish().samples.size();
        g.runLaps(5, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 5u);

        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 4000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        EXPECT_FALSE(anyEventBetween(r, tr.startIndex, ranFrom)) << "seed " << seed;
        // Standing still adds no distance, whatever GPS distance did.
        EXPECT_EQ(r.total[ranFrom - 1], 0.0f) << "seed " << seed;
        EXPECT_NEAR(r.events.back().totalM, 2000.0f, 0.01f);
    }
}

TEST(LapCounter, JoggingBackAndForthAcrossTheLine)
{
    const float amplitudes[] = {10.0f, 30.0f, 45.0f};
    for (float amp : amplitudes) {
        for (uint32_t seed : {1u, 2u, 3u}) {
            Generator g(TraceGen::Track(), noisy(4.0f), seed);
            g.begin(0.1f, 1);
            g.shuttle(240.0, amp, 2.5f);
            const size_t ranFrom = g.finish().samples.size();
            g.runLaps(4, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();

            const Replay::Result r = Replay::run(tr);
            EXPECT_FALSE(anyEventBetween(r, tr.startIndex, ranFrom)) << "amp " << amp << " seed " << seed;
            EXPECT_TRUE(wholeLapsRight(tr, r, 4000)) << "amp " << amp << " seed " << seed;
            EXPECT_EQ(r.missed, 0) << "amp " << amp << " seed " << seed;
        }
    }
}

TEST(LapCounter, RunUpFromBehindTheLine)
{
    // Start, walk 30 m back behind the line, then run forward through it.
    Generator g(TraceGen::Track(), noisy(3.0f), 5);
    g.begin(0.15f, 1);
    g.shuttle(30.0 / 1.5, 30.0f, 1.5f, true);
    g.runLaps(4, runner());
    g.run(kTailM + 30.0f, runner());
    const Trace tr = g.finish();
    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 3000));
    EXPECT_EQ(r.missed, 0);
}

TEST(LapCounter, Resting3MinutesOnTheLineBetweenReps)
{
    for (uint32_t seed : kSeeds) {
        // GPS distance that piles up jitter while standing still: 3 minutes
        // is over 1.5 laps of it, which must not credit a lap.
        Generator g(TraceGen::Track(), noisy(4.0f, 0.1f, 2.5f), seed);
        g.begin(0.1f, 1);
        g.runLaps(2, runner());
        g.run(20.0, runner());            // eases up past the line
        const size_t restFrom = g.finish().samples.size();
        g.stand(180.0);
        const size_t restTo = g.finish().samples.size();
        g.run(2 * 400.0 - 20.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();

        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 4000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        EXPECT_FALSE(anyEventBetween(r, restFrom, restTo)) << "seed " << seed;
    }
}

// ---------------------------------------------------------------------------
// Dropouts
// ---------------------------------------------------------------------------

TEST(LapCounter, ThirtySecondDropoutMidLap)
{
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.run(2.3 * 400.0, runner());
        g.dropout(true);
        g.stand(0.0);
        const double t0 = g.secondsSinceStart();
        g.run(150.0, runner());           // about 30 s
        g.dropout(false);
        EXPECT_NEAR(g.secondsSinceStart() - t0, 30.0, 3.0);
        g.run(3.7 * 400.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 6u);

        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        EXPECT_NEAR(r.events.back().totalM, 2400.0f, 0.01f);
    }
}

TEST(LapCounter, ThirtySecondDropoutSpanningTheGate)
{
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.run(3 * 400.0 - 75.0, runner());
        g.dropout(true);
        g.run(150.0, runner());           // about 30 s, centred on the line
        g.dropout(false);
        g.run(3 * 400.0 - 75.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 6u);

        const Replay::Result r = Replay::run(tr);
        // The missed lap's end is interpolated across the gap, so it is
        // close to the truth too.
        EXPECT_TRUE(wholeLapsRight(tr, r, 5000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 1) << "seed " << seed;
        ASSERT_EQ(r.events.size(), 6u);
        EXPECT_EQ(r.events[2].type, LapCounter::EventType::MissedLap) << "seed " << seed;
        EXPECT_NEAR(r.events[2].lapLengthM, 400.0f, 1e-3f);
        EXPECT_NEAR(r.events.back().totalM, 2400.0f, 0.01f);
    }
}

TEST(LapCounter, LongDropoutHidingTheFarSideAndTheGate)
{
    // 70 s without a fix, from the end of the first bend round to the same
    // bend a lap later: the readings either side do not straddle the gate
    // line at all. If GPS distance keeps counting through the gap (the
    // kernel dead-reckoning), the lap is credited at the next crossing.
    // If it stays flat, nothing can tell a lap went by: see docs/OVAL.md.
    for (uint32_t seed : kSeeds) {
        TraceGen::Noise n = noisy(3.0f);
        n.dropoutDistance = true;
        Generator g(TraceGen::Track(), n, seed);
        g.begin(0.1f, 1);
        g.run(2 * 400.0 - 250.0, runner());
        g.dropout(true);
        g.run(350.0, runner());
        g.dropout(false);
        g.run(3 * 400.0 - 100.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 5u);
        const Replay::Result r = Replay::run(tr);
        // The credited lap is timed where GPS distance said one lap.
        EXPECT_TRUE(wholeLapsRight(tr, r, 15000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 1) << "seed " << seed;
        EXPECT_NEAR(r.events.back().totalM, 2000.0f, 0.01f);
    }
}

TEST(LapCounter, PoorHdopReadingsAreIgnored)
{
    for (uint32_t seed : kSeeds) {
        TraceGen::Noise n = noisy(3.0f);
        n.badFixProb   = 0.15f;  // HDOP 6 to 10, with 15 m extra noise
        n.badFixSigmaM = 15.0f;
        Generator g(TraceGen::Track(), n, seed);
        g.begin(0.1f, 1);
        g.runLaps(8, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 5000)) << "seed " << seed;
    }
}

// ---------------------------------------------------------------------------
// Lanes
// ---------------------------------------------------------------------------

TEST(LapCounter, Lanes1To8)
{
    const LaneTable table = LaneTable::outdoor();
    for (uint8_t lane = 1; lane <= 8; ++lane) {
        Generator g(TraceGen::Track(), noisy(3.0f), 200 + lane);
        g.begin(0.12f, lane);
        g.runLaps(4, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr, table);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "lane " << int(lane);
        ASSERT_EQ(r.events.size(), 4u);
        EXPECT_NEAR(r.events.back().totalM, 4.0f * table.lapLengthM(lane), 0.01f) << "lane " << int(lane);
    }
}

TEST(LapCounter, LaneChangeMidLap)
{
    struct Case { uint8_t from; uint8_t to; };
    const Case cases[] = {{1, 4}, {8, 1}, {3, 5}, {2, 1}};
    const LaneTable table = LaneTable::outdoor();
    for (const Case& c : cases) {
        Generator g(TraceGen::Track(), noisy(3.0f), 300 + c.from * 10 + c.to);
        g.begin(0.1f, c.from);
        g.runLaps(1.5, runner());          // to the back straight
        g.changeLane(c.to);
        g.runLaps(2.5, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 4u);

        const Replay::Result r = Replay::run(tr, table);
        EXPECT_EQ(r.decreases, 0u);
        ASSERT_EQ(r.events.size(), 4u) << c.from << " to " << c.to;
        EXPECT_EQ(r.missed, 0);
        EXPECT_NEAR(r.events[0].lapLengthM, table.lapLengthM(c.from), 1e-3f);
        // The lap with the change: f x old + (1 - f) x new, with f the
        // estimated fraction at the change. Within a couple of metres of the
        // truth, and between the two lanes' lengths.
        EXPECT_NEAR(r.events[1].lapLengthM, tr.lapCreditM[1], 2.0) << c.from << " to " << c.to;
        EXPECT_NEAR(r.events[2].lapLengthM, table.lapLengthM(c.to), 1e-3f);
        EXPECT_NEAR(r.events[3].lapLengthM, table.lapLengthM(c.to), 1e-3f);
        EXPECT_NEAR(r.events.back().totalM, Replay::creditedAfter(tr, 4), 2.0);
        const Replay::Match m = Replay::match(tr, r, 3000);
        EXPECT_EQ(m.badTimes, 0u);
    }
}

TEST(LapCounter, ALaneChangeAfterAnOutAndBackCreditsWhatWasRun)
{
    // A stride 60 m out and back to the start, then lap 1 with a lane
    // change 60 m in: the lap is what was run on each lane; the stride shown
    // before it is not credited again.
    const LaneTable table = LaneTable::outdoor();
    for (uint32_t seed : kSeeds) {
        Generator g(TraceGen::Track(), noisy(3.0f), 600 + seed);
        g.begin(0.1f, 1);
        g.shuttle(30.0, 60.0f, 4.0f);
        g.run(60.0, runner());
        g.changeLane(4);
        g.runLaps(2, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr, table);
        ASSERT_GE(r.events.size(), 1u) << "seed " << seed;
        ASSERT_FALSE(tr.lapCreditM.empty());
        EXPECT_NEAR(r.events[0].lapLengthM, tr.lapCreditM[0], 3.0) << "seed " << seed;
    }
}

TEST(LapCounter, AHiddenLapDoesNotSkewTheGpsRatio)
{
    // A dropout over the line, the GPS distance flat through it and then the
    // chord: the next crossing is past 1.5 laps by distance, and the lap
    // the gap hid is credited. The lap after it has only what was left over
    // by distance, which says nothing of the ratio.
    for (uint32_t seed : kSeeds) {
        TraceGen::Noise n = noisy(3.0f);
        n.dropoutDistance = false;
        auto make = [&](bool gap) {
            Generator g(TraceGen::Track(), n, 700 + seed);
            g.begin(0.1f, 1);
            g.runLaps(1, runner());
            g.run(260.0, runner());
            g.dropout(gap);
            g.run(190.0, runner());
            g.dropout(false);
            g.run(350.0, runner());
            g.run(kTailM, runner());
            return g.finish();
        };
        const Replay::Result r = Replay::run(make(true));
        const Replay::Result c = Replay::run(make(false));
        EXPECT_EQ(r.missed, 1) << "seed " << seed;
        EXPECT_NEAR(r.gpsRatio, c.gpsRatio, 0.05) << "seed " << seed;
    }
}

TEST(LapCounter, LaneChangeBeforeTheGateIsSet)
{
    // Lane chosen wrongly on the pre-run screen and fixed at once.
    const LaneTable table = LaneTable::outdoor();
    Generator g(TraceGen::Track(), noisy(3.0f), 77);
    g.begin(0.1f, 3);
    g.changeLane(3);
    g.runLaps(3, runner());
    g.run(kTailM, runner());
    Trace tr = g.finish();
    tr.startLane = 1;   // started in the wrong lane, changed to 3 straight away
    const Replay::Result r = Replay::run(tr, table);
    ASSERT_EQ(r.events.size(), 3u);
    EXPECT_NEAR(r.events.back().totalM, 3.0f * table.lapLengthM(3), 0.01f);
}

// ---------------------------------------------------------------------------
// Pause and resume
// ---------------------------------------------------------------------------

TEST(LapCounter, PauseMidLapFreezesDistanceAndTime)
{
    Generator g(TraceGen::Track(), noisy(3.0f), 31);
    g.begin(0.1f, 1);
    g.runLaps(1.5, runner());
    g.pause();
    const size_t pausedFrom = g.finish().samples.size();
    g.run(50.0, steady(1.2f));             // walks on while paused
    g.stand(60.0);
    const size_t pausedTo = g.finish().samples.size();
    g.resume();
    g.run(2.5 * 400.0 - 50.0, runner());
    g.run(kTailM, runner());
    const Trace tr = g.finish();
    ASSERT_EQ(tr.crossMs.size(), 4u);

    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 3000));
    EXPECT_EQ(r.missed, 0);
    EXPECT_FALSE(anyEventBetween(r, pausedFrom, pausedTo));
    for (size_t i = pausedFrom + 1; i < pausedTo; ++i) {
        EXPECT_EQ(r.total[i], r.total[pausedFrom]) << "reading " << i;
    }
    // Lap 2 holds the pause: its duration is active time only.
    const Replay::Match m = Replay::match(tr, r, 3000);
    ASSERT_EQ(m.lapTimeErrMs.size(), 4u);
    EXPECT_LT(std::fabs(m.lapTimeErrMs[1]), 3000.0);
    EXPECT_LT(r.events[1].lapDurationMs, 110000u); // about 80 s running, not 80 s + 100 s paused
}

TEST(LapCounter, WalkingAcrossTheGateWhilePaused)
{
    for (uint32_t seed : kSeeds) {
        // Pause 8 m before the line, walk 16 m over it, resume on the far
        // side. That pass is a real lap, but it cannot count while paused: it
        // is credited later as a missed lap, timed at the pause.
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.run(2 * 400.0 - 8.0, runner());
        g.pause();
        const size_t pausedFrom = g.finish().samples.size();
        g.run(16.0, steady(1.2f));
        const size_t pausedTo = g.finish().samples.size();
        g.resume();
        g.run(3 * 400.0 - 8.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), 5u);

        const Replay::Result r = Replay::run(tr);
        EXPECT_EQ(r.decreases, 0u);
        EXPECT_FALSE(anyEventBetween(r, pausedFrom, pausedTo)) << "seed " << seed;
        ASSERT_EQ(r.events.size(), 5u) << "seed " << seed;
        EXPECT_EQ(r.missed, 1) << "seed " << seed;
        EXPECT_EQ(r.events[1].type, LapCounter::EventType::MissedLap);
        EXPECT_NEAR(r.events.back().totalM, 2000.0f, 0.01f);
        for (size_t k = 2; k < 5; ++k) {
            EXPECT_EQ(r.events[k].type, LapCounter::EventType::Lap);
        }
    }
}

TEST(LapCounter, WalkingBackAndForthOverTheGateWhilePaused)
{
    for (uint32_t seed : {1u, 2u, 3u}) {
        // Finish a rep, ease up past the line, pause, and wander back and
        // forth over it.
        Generator g(TraceGen::Track(), noisy(3.0f), seed);
        g.begin(0.1f, 1);
        g.run(2 * 400.0 + 20.0, runner());
        g.pause();
        const size_t pausedFrom = g.finish().samples.size();
        g.shuttle(90.0, 25.0f, 1.0f, true);
        const size_t pausedTo = g.finish().samples.size();
        g.resume();
        g.run(2 * 400.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();

        const Replay::Result r = Replay::run(tr);
        EXPECT_FALSE(anyEventBetween(r, pausedFrom, pausedTo)) << "seed " << seed;
        EXPECT_TRUE(wholeLapsRight(tr, r, 4000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
    }
}

// ---------------------------------------------------------------------------
// Leaving the track
// ---------------------------------------------------------------------------

TEST(LapCounter, Detour200mThenRejoin)
{
    const double where[] = {0.25, 0.5, 0.8};
    for (double at : where) {
        for (uint32_t seed : {1u, 2u, 3u}) {
            Generator g(TraceGen::Track(), noisy(3.0f), seed);
            g.begin(0.1f, 1);
            g.run((3.0 + at) * 400.0, runner());
            g.detour(100.0f, 4.0f);            // 100 m out and back
            g.run((3.0 - at) * 400.0, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            ASSERT_EQ(tr.crossMs.size(), 6u);

            const Replay::Result r = Replay::run(tr);
            EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "at " << at << " seed " << seed;
            EXPECT_EQ(r.missed, 0) << "at " << at << " seed " << seed;
            EXPECT_NEAR(r.events.back().totalM, 2400.0f, 0.01f);
        }
    }
}

TEST(LapCounter, StalePrimedStartPointIsReplaced)
{
    // The whole GPS track jumps sideways at Start, as when a fix is still
    // settling: the primed start point is off the runner's line. Up to
    // staleStartM (15 m) it stays, and crossings still fall inside
    // gateRadiusM; beyond, the first readings after Start replace it.
    const float shifts[] = {12.0f, 20.0f, 32.0f, 45.0f};
    for (float shift : shifts) {
        for (float dir : {1.0f, -1.0f}) { // into the infield, and out
            TraceGen::Noise n = noisy(2.0f, 0.02f);
            n.shiftNorthM = dir * shift; // across the home straight
            Generator g(TraceGen::Track(), n, 61);
            g.begin(0.1f, 1);
            g.runLaps(4, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::Result r = Replay::run(tr);
            EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "shift " << dir * shift;
            EXPECT_EQ(r.missed, 0) << "shift " << dir * shift;
        }
    }
}

TEST(LapCounter, FinishCreditsAPassStillPending)
{
    // The last lap ends with the runner pausing just before the line and
    // walking over it, then pressing Stop: that lap is credited at Stop.
    Generator g(TraceGen::Track(), noisy(2.0f), 62);
    g.begin(0.1f, 1);
    g.run(3 * 400.0 - 8.0, runner());
    g.pause();
    g.run(20.0, steady(1.2f));
    const Trace tr = g.finish();
    ASSERT_EQ(tr.crossMs.size(), 3u);

    LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs, 1, LaneTable::outdoor());
    size_t nextCmd = 0;
    NeverDecreases total;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            c.pause(tr.commands[nextCmd].tMs);
        }
        c.update(tr.samples[i]);
        total.check(c.totalDistanceM());
    }
    EXPECT_EQ(c.lapsCompleted(), 2);
    EXPECT_EQ(c.gateState(), LapCounter::GateState::Passed);
    const LapCounter::Events ev = c.finish();
    ASSERT_EQ(ev.count, 1);
    EXPECT_EQ(ev.items[0].type, LapCounter::EventType::MissedLap);
    EXPECT_EQ(ev.items[0].lapNumber, 3);
    EXPECT_NEAR(ev.items[0].totalM, 1200.0f, 0.01f);
    EXPECT_LT(std::abs(Oval::msDiff(ev.items[0].tMs, tr.crossMs[2])), 4000);
    EXPECT_EQ(c.finish().count, 0); // only once
    total.check(c.totalDistanceM());
    EXPECT_EQ(total.decreases, 0u);
}

// ---------------------------------------------------------------------------
// Clock
// ---------------------------------------------------------------------------

TEST(LapCounter, TimestampWrapAround)
{
    // The ms clock wraps 200 s into the run, in the middle of lap 3.
    Generator g(TraceGen::Track(), noisy(3.0f), 12);
    g.begin(0.1f, 1, 0xFFFFFFFFu - 200000u);
    g.runLaps(6, runner());
    g.run(kTailM, runner());
    const Trace tr = g.finish();
    ASSERT_GT(tr.samples.front().tMs, tr.samples.back().tMs) << "the trace wraps";

    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 3000));
    const Replay::Match m = Replay::match(tr, r, 3000);
    EXPECT_LT(worstAbs(m.lapTimeErrMs), 3000.0);
    for (const auto& e : r.events) {
        EXPECT_GT(e.lapDurationMs, 60000u);
        EXPECT_LT(e.lapDurationMs, 100000u);
    }
}

TEST(LapCounter, OutOfOrderAndDuplicateReadingsAreIgnored)
{
    Generator g(TraceGen::Track(), noisy(3.0f), 13);
    g.begin(0.1f, 1);
    g.runLaps(3, runner());
    g.run(kTailM, runner());
    Trace tr = g.finish();
    // Repeat every 20th reading, and put a stale one after it.
    std::vector<Oval::Sample> s;
    for (size_t i = 0; i < tr.samples.size(); ++i) {
        s.push_back(tr.samples[i]);
        if (i > tr.startIndex + 2 && i % 20 == 0) {
            s.push_back(tr.samples[i]);
            s.push_back(tr.samples[i - 2]);
        }
    }
    LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs, 1, LaneTable::outdoor());
    NeverDecreases total;
    for (size_t i = tr.startIndex; i < s.size(); ++i) {
        c.update(s[i]);
        total.check(c.totalDistanceM());
    }
    EXPECT_EQ(total.decreases, 0u);
    EXPECT_EQ(c.lapsCompleted(), 3);
    EXPECT_NEAR(c.creditedDistanceM(), 1200.0f, 0.01f);
}

// ---------------------------------------------------------------------------
// GPS distance: over-read, under-read, missing
// ---------------------------------------------------------------------------

TEST(LapCounter, SimulatorLikeOverReadOf12Percent)
{
    // The simulator: kerb-line runner at 15 to 25 km/h, slower on the bends,
    // 1.5 m white noise summed into GPS distance. A simulator run read 445 m a
    // lap, 12% high. Laps stay exact, and after lap 1 the part-lap estimate
    // is corrected by the ratio.
    for (uint32_t seed : kSeeds) {
        TraceGen::Noise n = noisy(1.0f, 0.075f, 1.5f);
        Generator g(TraceGen::Track(), n, seed);
        g.begin(0.0f, 1);
        // 20 km/h, three quarters of that on the bends, smoothed, as the
        // simulator's GpsStepCounterSimulator does.
        TraceGen::Speed sim;
        sim.baseMps    = 5.5f;
        sim.bendFactor = 0.75f;
        sim.jitter     = 0.01f;
        g.runLaps(9, sim);
        g.run(kTailM, sim);
        const Trace tr = g.finish();

        const double overRead = tr.gpsDistM / tr.movedM;
        std::printf("[ info     ] seed %u: simulator-like GPS distance reads %.1f%% high (%.0f m a lap)\n",
                    seed, 100.0 * (overRead - 1.0), 400.0 * overRead);
        EXPECT_GT(overRead, 1.08) << "seed " << seed;
        EXPECT_LT(overRead, 1.18) << "seed " << seed;

        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 2000)) << "seed " << seed;
        EXPECT_EQ(r.missed, 0) << "seed " << seed;
        // The ratio is learnt from the last lap alone, so it is checked against
        // that lap's own figure, from the GPS distance between its two true
        // crossings (against the whole run's average, a lap's own
        // variation of about 4% failed it on most draws).
        auto gpsAt = [&](uint32_t t) {
            for (size_t i = 1; i < tr.samples.size(); ++i) {
                if (Oval::msDiff(tr.samples[i].tMs, t) >= 0) {
                    const auto& a = tr.samples[i - 1];
                    const auto& b = tr.samples[i];
                    const double f = static_cast<double>(Oval::msDiff(t, a.tMs)) / Oval::msDiff(b.tMs, a.tMs);
                    return a.gpsDistM + f * (b.gpsDistM - a.gpsDistM);
                }
            }
            return static_cast<double>(tr.samples.back().gpsDistM);
        };
        const size_t nc = tr.crossMs.size();
        const double lastLapRatio = 400.0 / (gpsAt(tr.crossMs[nc - 1]) - gpsAt(tr.crossMs[nc - 2]));
        std::printf("[ info     ] seed %u: GPS ratio %.4f; whole run %.4f, last lap %.4f\n", seed, r.gpsRatio,
                    1.0 / overRead, lastLapRatio);
        EXPECT_NEAR(r.gpsRatio, lastLapRatio, 0.02) << "seed " << seed;

        // Part-lap error against the truth, away from the crossings (within
        // a few seconds of one, the error is the crossing's timing, not the
        // estimate). The uncorrected 12% shows in lap 1, up to the 98% cap;
        // from lap 2 the ratio corrects it. What is left is the GPS distance
        // wandering within each lap: summed 1.5 m white jitter varies by
        // about 19 m a lap, which no ratio can predict.
        double sumLap1 = 0.0, sumAfter = 0.0, worstAfter = 0.0;
        size_t nLap1 = 0, nAfter = 0;
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            bool nearCrossing = false;
            for (uint32_t c : tr.crossMs) {
                nearCrossing = nearCrossing || std::abs(Oval::msDiff(tr.samples[i].tMs, c)) < 5000;
            }
            if (nearCrossing) {
                continue;
            }
            const double err = std::fabs(r.total[i] - tr.trueTrackM[i]);
            if (Oval::msDiff(tr.samples[i].tMs, tr.crossMs[0]) < 0) {
                sumLap1 += err;
                ++nLap1;
            } else {
                sumAfter += err;
                ++nAfter;
                worstAfter = std::max(worstAfter, err);
            }
        }
        // The same readings with no ratio: raw GPS distance since the lap
        // began, under the same 98% cap, for laps 2 to 9.
        double sumRaw = 0.0;
        size_t nRaw = 0;
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            bool nearCrossing = false;
            for (uint32_t c : tr.crossMs) {
                nearCrossing = nearCrossing || std::abs(Oval::msDiff(tr.samples[i].tMs, c)) < 5000;
            }
            size_t k = 0;
            while (k < r.events.size() && r.eventSample[k] <= i) {
                ++k;
            }
            if (nearCrossing || k == 0) {
                continue;
            }
            const double part = std::min(tr.samples[i].gpsDistM - gpsAt(r.events[k - 1].tMs), 0.98 * 400.0);
            sumRaw += std::fabs(r.events[k - 1].totalM + part - tr.trueTrackM[i]);
            ++nRaw;
        }
        std::printf("[ info     ] seed %u: laps 2 to 9 with no ratio, mean %.1f m\n", seed, sumRaw / nRaw);
        const double meanLap1 = sumLap1 / nLap1, meanAfter = sumAfter / nAfter;
        std::printf("[ info     ] seed %u: part-lap error, lap 1 mean %.1f m; laps 2 to 9 mean %.1f m, worst %.1f m\n",
                    seed, meanLap1, meanAfter, worstAfter);
        // The ratio at least halves the error the same readings would have had
        // without it (lap 1's error alone passes or fails by the luck of the
        // draw).
        EXPECT_GT(meanLap1, 10.0) << "seed " << seed;
        EXPECT_LT(meanAfter, 0.5 * sumRaw / nRaw) << "seed " << seed;
        EXPECT_LT(meanAfter, 10.0) << "seed " << seed;
        EXPECT_LT(worstAfter, 40.0) << "seed " << seed;
    }
}

TEST(LapCounter, OverReadAndUnderReadAreCorrectedWithinTheClamp)
{
    const float scales[] = {0.85f, 0.95f, 1.12f, 1.2f};
    for (float scale : scales) {
        TraceGen::Noise n = noisy(3.0f, 0.1f, 0.5f);
        n.distScale = scale;
        Generator g(TraceGen::Track(), n, 50);
        g.begin(0.1f, 1);
        g.runLaps(6, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "scale " << scale;
        EXPECT_NEAR(r.gpsRatio, 1.0f / scale, 0.03f) << "scale " << scale;
    }
}

TEST(LapCounter, RatioIsClampedTo0_8And1_25)
{
    {
        TraceGen::Noise n = clean();
        n.distScale = 1.6f;
        Generator g(TraceGen::Track(), n, 51);
        g.begin(0.1f, 1);
        g.runLaps(4, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 1000));
        EXPECT_FLOAT_EQ(r.gpsRatio, 0.8f);
    }
    {
        TraceGen::Noise n = clean();
        n.distScale = 0.75f;
        Generator g(TraceGen::Track(), n, 52);
        g.begin(0.1f, 1);
        g.runLaps(4, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 1000));
        EXPECT_FLOAT_EQ(r.gpsRatio, 1.25f);
    }
}

TEST(LapCounter, PartLapEstimateIsCappedAt98Percent)
{
    TraceGen::Noise n = clean();
    n.distScale = 1.3f; // lap 1 is uncorrected: the estimate hits the cap
    Generator g(TraceGen::Track(), n, 53);
    g.begin(0.1f, 2);
    g.runLaps(3, runner());
    g.run(kTailM, runner());
    const Trace tr = g.finish();

    const LaneTable table = LaneTable::outdoor();
    LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs, 2, table);
    float worst = 0.0f;
    NeverDecreases total;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        c.update(tr.samples[i]);
        total.check(c.totalDistanceM());
        worst = std::max(worst, c.lapProgressM());
        EXPECT_LE(c.lapProgressM(), 0.98f * table.lapLengthM(2) + 1e-3f);
    }
    EXPECT_NEAR(worst, 0.98f * table.lapLengthM(2), 0.01f);
    EXPECT_EQ(c.lapsCompleted(), 3);
    EXPECT_EQ(total.decreases, 0u);
}

TEST(LapCounter, NoGpsDistanceFallsBackToSummedSteps)
{
    for (float sigma : {0.0f, 2.0f}) {
        TraceGen::Noise n = sigma > 0.0f ? noisy(sigma, 0.05f) : clean();
        n.gpsDist = false;
        Generator g(TraceGen::Track(), n, 54);
        g.begin(0.1f, 1);
        g.runLaps(5, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::Result r = Replay::run(tr);
        EXPECT_TRUE(wholeLapsRight(tr, r, 3000)) << "sigma " << sigma;
        EXPECT_EQ(r.missed, 0) << "sigma " << sigma;
        if (sigma == 0.0f) {
            EXPECT_NEAR(r.gpsRatio, 1.0f, 0.01f);
        }
        // The estimate still moves between crossings.
        EXPECT_GT(r.total[tr.startIndex + 30], 50.0f);
    }
}

TEST(LapCounter, GpsDistanceGlitchIsCutToMaxSpeed)
{
    Generator g(TraceGen::Track(), noisy(2.0f), 55);
    g.begin(0.1f, 1);
    g.runLaps(4, runner());
    g.run(kTailM, runner());
    Trace tr = g.finish();
    // A one-off 1 km jump in the kernel's distance, half way round lap 2.
    const size_t glitch = tr.startIndex + 120;
    for (size_t i = glitch; i < tr.samples.size(); ++i) {
        tr.samples[i].gpsDistM += 1000.0f;
    }
    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 3000));
    EXPECT_EQ(r.missed, 0);
}

// ---------------------------------------------------------------------------
// Start point
// ---------------------------------------------------------------------------

TEST(LapCounter, StartPointIsTheHdopWeightedMeanOfRecentFixes)
{
    // The figure is HDOP, weighted by 1 / (HDOP x uereM) squared: HDOP 0.5, 0.5 and 3.75 are errors of 2, 2 and 15 m.
    Oval::LocalFrame ref;
    ref.setOrigin(55.95, -3.19);
    auto at = [&](float x, float y, uint32_t t, float hdop) {
        Oval::Sample s;
        ref.toLatLon({x, y}, s.lat, s.lon);
        s.tMs = t;
        s.fix = true;
        s.hdop = hdop;
        return s;
    };
    LapCounter c;
    c.prime(at(500.0f, 0.0f, 1000, 0.75f));  // too old by Start
    c.prime(at(0.0f, 0.0f, 9000, 0.5f));
    c.prime(at(2.0f, 0.0f, 10000, 0.5f));
    c.prime(at(40.0f, 0.0f, 11000, 3.75f));  // poor: weight 1/225 against 1/4
    Oval::Sample bad = at(-300.0f, 0.0f, 11500, 5.0f);
    c.prime(bad);                            // worse than HDOP 4: ignored
    bad.fix = false;
    c.prime(bad);
    ASSERT_TRUE(c.start(12000, 1, LaneTable::outdoor()));
    EXPECT_EQ(c.gateState(), LapCounter::GateState::NeedGate);
    ASSERT_TRUE(c.frame().hasOrigin());
    const Oval::Vec2 o = ref.toLocal(c.frame().originLat(), c.frame().originLon());
    // (0 x 0.25 + 2 x 0.25 + 40 / 225) / (0.5 + 1 / 225)
    EXPECT_NEAR(o.x, (2.0f * 0.25f + 40.0f / 225.0f) / (0.5f + 1.0f / 225.0f), 0.05f);
    EXPECT_NEAR(o.y, 0.0f, 0.05f);
}

TEST(LapCounter, PrimedStartPointBeatsASingleFix)
{
    double sumErr = 0.0;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        Generator g(TraceGen::Track(), noisy(5.0f, 0.0f), seed);
        g.begin(0.1f, 1, 1000000u, 6);
        g.runLaps(1, runner());
        const Trace tr = g.finish();
        LapCounter c;
        for (size_t i = 0; i < tr.startIndex; ++i) {
            c.prime(tr.samples[i]);
        }
        c.start(tr.startMs, 1, LaneTable::outdoor());
        Oval::LocalFrame ref;
        ref.setOrigin(tr.startLat, tr.startLon);
        sumErr += Oval::length(ref.toLocal(c.frame().originLat(), c.frame().originLon()));
    }
    // One fix with 5 m per axis is 6.3 m out on average; six, about 2.6 m.
    EXPECT_LT(sumErr / 20.0, 4.0);
}

TEST(LapCounter, WithoutPrimedFixesTheFirstFixAfterStartIsUsed)
{
    Generator g(TraceGen::Track(), noisy(3.0f), 56);
    g.begin(0.1f, 1, 1000000u, 0);
    g.runLaps(4, runner());
    g.run(kTailM, runner());
    const Trace tr = g.finish();

    LapCounter c;
    c.start(tr.startMs, 1, LaneTable::outdoor());
    EXPECT_EQ(c.gateState(), LapCounter::GateState::NeedFix);
    EXPECT_FALSE(c.frame().hasOrigin());
    const Replay::Result r = Replay::run(tr);
    EXPECT_TRUE(wholeLapsRight(tr, r, 3000));
}

TEST(LapCounter, StalePrimedFixesAreIgnored)
{
    Generator g(TraceGen::Track(), noisy(3.0f), 57);
    g.begin(0.1f, 1);
    const Trace tr = g.finish();
    LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs + 60000u, 1, LaneTable::outdoor()); // a minute later
    EXPECT_EQ(c.gateState(), LapCounter::GateState::NeedFix);
}

// ---------------------------------------------------------------------------
// API details
// ---------------------------------------------------------------------------

TEST(LapCounter, RejectsInvalidLanes)
{
    LapCounter c;
    EXPECT_FALSE(c.start(0, 0, LaneTable::outdoor()));
    EXPECT_FALSE(c.start(0, 9, LaneTable::outdoor()));
    EXPECT_FALSE(c.isRunning());
    ASSERT_TRUE(c.start(0, 8, LaneTable::outdoor()));
    EXPECT_FALSE(c.setLane(0));
    EXPECT_FALSE(c.setLane(9));
    EXPECT_EQ(c.lane(), 8);
    EXPECT_TRUE(c.setLane(2));
    EXPECT_EQ(c.lane(), 2);
    EXPECT_NEAR(c.laneLengthM(), 407.04f, 0.01f);
}

TEST(LapCounter, NothingHappensBeforeStart)
{
    Generator g(TraceGen::Track(), noisy(3.0f), 58);
    g.begin(0.1f, 1);
    g.runLaps(2, runner());
    const Trace tr = g.finish();
    LapCounter c;
    for (const auto& s : tr.samples) {
        const auto ev = c.update(s);
        EXPECT_EQ(ev.count, 0);
    }
    EXPECT_EQ(c.lapsCompleted(), 0);
    EXPECT_EQ(c.totalDistanceM(), 0.0f);
    EXPECT_EQ(c.gateState(), LapCounter::GateState::Idle);
}

TEST(LapCounter, GettersTrackTheCurrentLap)
{
    Generator g(TraceGen::Track(), clean(), 59);
    g.begin(0.1f, 1);
    g.runLaps(2, steady(5.0f));
    g.run(100.0, steady(5.0f));
    const Trace tr = g.finish();

    LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs, 1, LaneTable::outdoor());
    EXPECT_TRUE(c.isRunning());
    NeverDecreases total;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        c.update(tr.samples[i]);
        total.check(c.totalDistanceM());
    }
    EXPECT_EQ(total.decreases, 0u);
    EXPECT_EQ(c.lapsCompleted(), 2);
    EXPECT_EQ(c.missedLapsCredited(), 0);
    EXPECT_NEAR(c.lastLapDurationMs(), 80000.0, 200.0);    // 400 m at 5 m/s
    EXPECT_NEAR(c.lapElapsedMs(), 20000.0, 1200.0);        // 100 m into lap 3
    EXPECT_NEAR(c.lapProgressM(), 100.0f, 6.0f);
    EXPECT_NEAR(c.totalDistanceM(), 900.0f, 6.0f);
    EXPECT_NEAR(c.creditedDistanceM(), 800.0f, 0.01f);
    EXPECT_EQ(c.lapElapsedMs(tr.samples.back().tMs + 5000u), c.lapElapsedMs() + 5000u);
    EXPECT_TRUE(c.hasGate());
    EXPECT_EQ(c.gateState(), LapCounter::GateState::Out);

    c.pause(tr.samples.back().tMs + 1000u);
    EXPECT_TRUE(c.isPaused());
    EXPECT_EQ(c.lapElapsedMs(tr.samples.back().tMs + 60000u), c.lapElapsedMs(tr.samples.back().tMs + 1000u));

    c.reset();
    EXPECT_FALSE(c.isRunning());
    EXPECT_EQ(c.lapsCompleted(), 0);
    EXPECT_EQ(c.totalDistanceM(), 0.0f);
}

TEST(LapCounter, ConfigChangesTheGuards)
{
    // A tighter time guard than the lap time blocks every crossing; the laps
    // are then only credited by distance, so they are late but not lost.
    Oval::Config cfg;
    cfg.minLapMs = 90000; // laps take 80 s
    Generator g(TraceGen::Track(), clean(), 60);
    g.begin(0.1f, 1);
    g.runLaps(3, steady(5.0f));
    g.run(kTailM, steady(5.0f));
    const Trace tr = g.finish();
    const Replay::Result strict = Replay::run(tr, LaneTable::outdoor(), cfg);
    const Replay::Result normal = Replay::run(tr);
    EXPECT_EQ(normal.laps, 3);
    EXPECT_LT(strict.laps, 3);
    EXPECT_EQ(strict.decreases, 0u);
}
