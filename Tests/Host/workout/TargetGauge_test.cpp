/**
 ******************************************************************************
 * @file    TargetGauge_test.cpp
 * @brief   Tests for SDK::Workout::SpeedGauge, HeartRateGauge,
 *          ZoneHysteresis and arcFraction().
 *
 * Runs are synthetic, generated per second from a seeded random number
 * generator as a test model: the live speed is the true speed with some lag
 * and noise, and the distance carries a bounded error, so each second's
 * distance jitters while the total does not drift.
 ******************************************************************************
 */

#include "SDK/Workout/TargetGauge.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace SDK::Workout;

namespace {

// A synthetic run: what the gauge is fed each second.
struct Sample {
    float    live;       // live speed, m/s
    bool     liveValid;
    uint32_t distCm;     // step distance so far
};

struct SyntheticRun {
    std::mt19937 rng;
    explicit SyntheticRun(uint32_t seed) : rng(seed) {}

    // @p truth: the true speed for each second, m/s.
    std::vector<Sample> make(const std::vector<float>& truth, float liveNoise = 0.25f,
                             float positionNoiseM = 1.0f, int lagSec = 2)
    {
        std::normal_distribution<float> n(0.0f, 1.0f);
        const float livePhi = 0.9f;  // the live noise wanders slowly
        const float posPhi  = 0.7f;  // so does the position error, around zero
        float wander = 0.0f, posErr = 0.0f;
        double trueDist = 0.0, measured = 0.0;
        std::vector<Sample> out;
        for (size_t i = 0; i < truth.size(); ++i) {
            wander = livePhi * wander + std::sqrt(1.0f - livePhi * livePhi) * liveNoise * n(rng);
            posErr = posPhi * posErr + std::sqrt(1.0f - posPhi * posPhi) * positionNoiseM * n(rng);
            const float lagged = truth[i >= static_cast<size_t>(lagSec) ? i - lagSec : 0];
            trueDist += truth[i];
            measured = std::max(measured, trueDist + posErr);  // a distance never goes back
            out.push_back({std::max(0.0f, lagged + wander), true,
                           static_cast<uint32_t>(measured * 100.0)});
        }
        return out;
    }
};

std::vector<float> constant(float mps, int seconds) { return std::vector<float>(seconds, mps); }

int zoneChanges(const std::vector<Zone>& zones)
{
    int n = 0;
    for (size_t i = 1; i < zones.size(); ++i) {
        if (zones[i] != zones[i - 1] && zones[i - 1] != Zone::Unknown) {
            ++n;
        }
    }
    return n;
}

// The band of a 3:45-4:05 /km step, as m/s.
constexpr float kLow  = 1000.0f / 245.0f;  // 4.08
constexpr float kHigh = 1000.0f / 225.0f;  // 4.44
constexpr float kMid  = 1000.0f / 235.0f;  // 4.26

}  // namespace

// --- ZoneHysteresis ------------------------------------------------------------

TEST(ZoneHysteresis, FirstZoneNeedsOnlyTheDwell)
{
    ZoneHysteresis h(3.0f, 2);
    EXPECT_EQ(h.update(10.0f, 0.0f, 20.0f), Zone::Unknown);
    EXPECT_EQ(h.update(10.0f, 0.0f, 20.0f), Zone::Unknown);
    EXPECT_EQ(h.update(10.0f, 0.0f, 20.0f), Zone::In);  // more than 2 in a row
}

TEST(ZoneHysteresis, LeavingAndReturningNeedTheDeadband)
{
    ZoneHysteresis h(3.0f, 0);  // no dwell, to see the deadband alone
    h.update(10.0f, 0.0f, 20.0f);
    ASSERT_EQ(h.zone(), Zone::In);
    EXPECT_EQ(h.update(22.9f, 0.0f, 20.0f), Zone::In);     // not 3 past the edge
    EXPECT_EQ(h.update(23.1f, 0.0f, 20.0f), Zone::Above);
    EXPECT_EQ(h.update(17.1f, 0.0f, 20.0f), Zone::Above);  // not 3 back inside
    EXPECT_EQ(h.update(16.9f, 0.0f, 20.0f), Zone::In);
    EXPECT_EQ(h.update(-3.1f, 0.0f, 20.0f), Zone::Below);
    EXPECT_EQ(h.update(23.5f, 0.0f, 20.0f), Zone::Above);  // straight across
}

TEST(ZoneHysteresis, NarrowBandCanBeReentered)
{
    ZoneHysteresis h(3.0f, 0);
    h.update(230.0f, 240.0f, 242.0f);
    ASSERT_EQ(h.zone(), Zone::Below);
    EXPECT_EQ(h.update(240.9f, 240.0f, 242.0f), Zone::Below);
    EXPECT_EQ(h.update(241.1f, 240.0f, 242.0f), Zone::In);  // past the middle
    EXPECT_EQ(h.update(244.9f, 240.0f, 242.0f), Zone::In);  // leaving keeps the deadband
    EXPECT_EQ(h.update(245.1f, 240.0f, 242.0f), Zone::Above);
    EXPECT_EQ(h.update(241.1f, 240.0f, 242.0f), Zone::Above);
    EXPECT_EQ(h.update(240.9f, 240.0f, 242.0f), Zone::In);
}

TEST(ZoneHysteresis, ZeroWidthBandIsReenteredOnReachingIt)
{
    ZoneHysteresis h(3.0f, 0);
    h.update(250.0f, 241.0f, 241.0f);
    ASSERT_EQ(h.zone(), Zone::Above);
    EXPECT_EQ(h.update(241.5f, 241.0f, 241.0f), Zone::Above);
    EXPECT_EQ(h.update(241.0f, 241.0f, 241.0f), Zone::In);
    EXPECT_EQ(h.update(230.0f, 241.0f, 241.0f), Zone::Below);
    EXPECT_EQ(h.update(241.0f, 241.0f, 241.0f), Zone::In);
}

TEST(ZoneHysteresis, DwellRestartsWhenTheCandidateChanges)
{
    ZoneHysteresis h(0.0f, 2);
    for (int i = 0; i < 3; ++i) h.update(10.0f, 0.0f, 20.0f);
    ASSERT_EQ(h.zone(), Zone::In);
    h.update(30.0f, 0.0f, 20.0f);
    h.update(-5.0f, 0.0f, 20.0f);
    h.update(30.0f, 0.0f, 20.0f);
    EXPECT_EQ(h.update(30.0f, 0.0f, 20.0f), Zone::In);  // only two Above in a row
    EXPECT_EQ(h.update(30.0f, 0.0f, 20.0f), Zone::Above);
}

TEST(ZoneHysteresis, DwellIgnoresShortExcursions)
{
    ZoneHysteresis h(0.0f, 2);
    for (int i = 0; i < 3; ++i) h.update(10.0f, 0.0f, 20.0f);
    ASSERT_EQ(h.zone(), Zone::In);
    h.update(30.0f, 0.0f, 20.0f);
    h.update(30.0f, 0.0f, 20.0f);
    EXPECT_EQ(h.update(10.0f, 0.0f, 20.0f), Zone::In);  // two outside, then back
    h.update(30.0f, 0.0f, 20.0f);
    h.update(30.0f, 0.0f, 20.0f);
    EXPECT_EQ(h.update(30.0f, 0.0f, 20.0f), Zone::Above);  // three in a row
}

// --- SpeedGauge ------------------------------------------------------------------

TEST(SpeedGauge, StartsFromLiveAndMovesToTheWindow)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    // Live says 5 m/s, the distance says 4 m/s.
    g.tick(5.0f, true, 400);
    const float w1 = std::exp(-1.0f / SpeedGauge::kBlendTauSec);
    EXPECT_NEAR(g.valueMps(), w1 * 5.0f + (1.0f - w1) * 4.0f, 1e-4f);
    uint32_t d = 400;
    for (int t = 2; t <= 300; ++t) {
        d += 400;
        g.tick(5.0f, true, d);
    }
    EXPECT_NEAR(g.valueMps(), 4.0f, 0.01f);  // live has all but gone
}

TEST(SpeedGauge, HoldsTheZoneOffAtTheStart)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 1; t <= SpeedGauge::kHoldOffSec; ++t) {
        d += 426;
        g.tick(kMid, true, d);
        EXPECT_EQ(g.zone(), Zone::Unknown) << t;
    }
    for (int t = 0; t < 3; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    EXPECT_EQ(g.zone(), Zone::In);
}

TEST(SpeedGauge, NoisyRunInTheBandStaysIn)
{
    for (uint32_t seed = 1; seed <= 10; ++seed) {
        SyntheticRun run(seed);
        const auto samples = run.make(constant(kMid, 600));
        SpeedGauge g;
        g.startStep(kLow, kHigh);
        std::vector<Zone> gauge, raw;
        for (const Sample& s : samples) {
            g.tick(s.live, s.liveValid, s.distCm);
            gauge.push_back(g.zone());
            raw.push_back(s.live < kLow ? Zone::Below : s.live > kHigh ? Zone::Above : Zone::In);
        }
        int in = 0;
        for (size_t i = 60; i < gauge.size(); ++i) in += gauge[i] == Zone::In;
        EXPECT_GT(in, static_cast<int>((gauge.size() - 60) * 9 / 10)) << "seed " << seed;
        // Far steadier than judging the live speed alone.
        EXPECT_LT(zoneChanges(gauge) * 5, zoneChanges(raw)) << "seed " << seed;
    }
}

TEST(SpeedGauge, TooFastAndTooSlowAreSeen)
{
    for (const auto& c : {std::make_pair(4.9f, Zone::Above), std::make_pair(3.6f, Zone::Below)}) {
        SyntheticRun run(2);
        const auto samples = run.make(constant(c.first, 120));
        SpeedGauge g;
        g.startStep(kLow, kHigh);
        for (const Sample& s : samples) g.tick(s.live, s.liveValid, s.distCm);
        EXPECT_EQ(g.zone(), c.second);
    }
}

TEST(SpeedGauge, FollowsAPaceChangeWithinAboutTheWindow)
{
    // A long step: 4 minutes in the band, then much too fast.
    std::vector<float> truth = constant(kMid, 240);
    const std::vector<float> fast = constant(5.0f, 120);
    truth.insert(truth.end(), fast.begin(), fast.end());
    SyntheticRun run(3);
    const auto samples = run.make(truth);
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    int changedAt = -1;
    for (size_t i = 0; i < samples.size(); ++i) {
        g.tick(samples[i].live, samples[i].liveValid, samples[i].distCm);
        if (i >= 240 && changedAt < 0 && g.zone() == Zone::Above) {
            changedAt = static_cast<int>(i - 240);
        }
    }
    ASSERT_GE(changedAt, 0);
    // 200 m at 5 m/s is 40 s: the window has turned over by then, while a
    // whole-step average would still be mostly the first 4 minutes.
    EXPECT_LE(changedAt, 45);
    EXPECT_LT(g.stepAverageMps(), 4.6f);
}

TEST(SpeedGauge, StepAverageIsDistanceOverTime)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    g.tick(4.0f, true, 300);
    g.tick(4.0f, true, 700);
    EXPECT_NEAR(g.stepAverageMps(), 3.5f, 1e-5f);
}

TEST(SpeedGauge, LostSignalHoldsTheValueAndRecoversWithAHoldOff)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 60; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    ASSERT_EQ(g.zone(), Zone::In);
    const float held = g.valueMps();
    for (int t = 0; t < 10; ++t) {
        g.tick(0.0f, false, d);  // no fix: distance stalls
        EXPECT_EQ(g.zone(), Zone::NoSignal);
        EXPECT_FLOAT_EQ(g.valueMps(), held);
    }
    // The fix returns, and the distance catches up over the gap.
    d += 426 * 11;
    g.tick(kMid, true, d);
    EXPECT_EQ(g.zone(), Zone::Unknown);  // held off again
    EXPECT_NEAR(g.valueMps(), kMid, 0.05f);
}

TEST(SpeedGauge, LongGapCatchUpIsAveragedOverTheGap)
{
    // A minute without a reading while running at 5 m/s, then the distance
    // catches up by 300 m in one second.
    SpeedGauge g;
    g.startStep(4.5f, 5.5f);
    uint32_t d = 0;
    for (int t = 0; t < 60; ++t) {
        d += 500;
        g.tick(5.0f, true, d);
    }
    for (int t = 0; t < 60; ++t) g.tick(0.0f, false, d);
    d += 500 * 61;
    // Live reads 5.6, so the value shows where the window starts: on the
    // gap's average, 5.0, only if it reaches back over the gap.
    for (int t = 1; t <= 60; ++t) {
        g.tick(5.6f, true, d);
        const float w = std::exp(-static_cast<float>(t) / SpeedGauge::kBlendTauSec);
        EXPECT_NEAR(g.valueMps(), w * 5.6f + (1.0f - w) * 5.0f, 1e-3f) << t;
        d += 500;
    }
}

TEST(SpeedGauge, GapAcrossAStepStartStaysOutOfTheNewStep)
{
    // In band at 4.26 m/s. The signal goes 5 s before the step ends and
    // returns 4 s into the next, when its distance catches up by 10 s of
    // running in one go.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 115; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    for (int t = 0; t < 5; ++t) g.tick(0.0f, false, d);
    g.startStep(kLow, kHigh);
    for (int t = 0; t < 4; ++t) g.tick(0.0f, false, 0);
    d = 426 * 10;
    for (int t = 0; t < 120; ++t) {
        g.tick(kMid, true, d);
        EXPECT_NEAR(g.valueMps(), kMid, 0.05f) << t;
        EXPECT_NE(g.zone(), Zone::Above) << t;
        d += 426;
    }
}

TEST(SpeedGauge, GapStaysOpenAcrossStepsStartedWithoutATick)
{
    // Two steps start with no tick between them while the signal is lost,
    // as when R2 and a step's end come in the same second. The catch-up
    // still stays out of the window.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    g.tick(0.0f, false, 0);
    g.startStep(kLow, kHigh);
    g.startStep(kLow, kHigh);
    for (int t = 0; t < 10; ++t) g.tick(0.0f, false, 0);
    uint32_t d = 5000;  // the catch-up
    g.tick(5.0f, true, d);
    d += 400;
    g.tick(5.0f, true, d);
    const float w = std::exp(-2.0f / SpeedGauge::kBlendTauSec);
    EXPECT_NEAR(g.valueMps(), w * 5.0f + (1.0f - w) * 4.0f, 1e-3f);
}

TEST(SpeedGauge, WithNothingBeforeAGapTheWindowStartsAfterIt)
{
    // The signal is lost before the step starts, so nothing before the gap
    // is in the step. Once it returns the window starts on its first second,
    // so it averages the running since then and leaves out the catch-up.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    g.tick(0.0f, false, 0);
    g.startStep(kLow, kHigh);
    for (int t = 0; t < 10; ++t) g.tick(0.0f, false, 0);
    uint32_t d = 5000;  // the catch-up
    g.tick(5.0f, true, d);
    EXPECT_FLOAT_EQ(g.valueMps(), 5.0f);  // nothing to average yet: live
    for (int t = 2; t <= 20; ++t) {
        d += 400;
        g.tick(5.0f, true, d);
        const float w = std::exp(-static_cast<float>(t) / SpeedGauge::kBlendTauSec);
        EXPECT_NEAR(g.valueMps(), w * 5.0f + (1.0f - w) * 4.0f, 1e-3f) << t;
    }
}

TEST(SpeedGauge, GapLongerThanTheHistoryFallsBackToLive)
{
    SpeedGauge g;
    g.startStep(4.5f, 5.5f);
    uint32_t d = 0;
    for (int t = 0; t < 30; ++t) {
        d += 500;
        g.tick(5.0f, true, d);
    }
    for (size_t t = 0; t < SpeedGauge::kHistory + 10; ++t) g.tick(0.0f, false, d);
    d += 500 * static_cast<uint32_t>(SpeedGauge::kHistory + 11);
    g.tick(5.0f, true, d);
    EXPECT_NEAR(g.valueMps(), 5.0f, 1e-4f);
}

TEST(SpeedGauge, StandingStillWithAReadingStaysInTheWindow)
{
    // A stop at a crossing, not paused: the window takes it at face value.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 100; ++t) {
        d += 400;
        g.tick(4.0f, true, d);
    }
    for (int t = 0; t < 20; ++t) g.tick(0.0f, true, d);
    d += 400;
    g.tick(4.0f, true, d);
    // The latest second at least 200 m back is 70 s ago, the stop included.
    const float window = 20000.0f / 100.0f / 70.0f;
    const float w = std::exp(-121.0f / SpeedGauge::kBlendTauSec);
    EXPECT_NEAR(g.valueMps(), w * 4.0f + (1.0f - w) * window, 1e-3f);
}

TEST(SpeedGauge, ResumeKeepsTheWindowButRestartsTheBlend)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 200; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    g.resume();
    EXPECT_EQ(g.zone(), Zone::Unknown);
    // Just after resuming, live reads 0 (no fresh sample yet); the window
    // still says the step's pace, and the blend starts from live.
    d += 426;
    g.tick(0.0f, true, d);
    const float w1 = std::exp(-1.0f / SpeedGauge::kBlendTauSec);
    EXPECT_NEAR(g.valueMps(), (1.0f - w1) * kMid, 0.02f);
    EXPECT_EQ(g.zone(), Zone::Unknown);  // and no false "too slow"
}

TEST(SpeedGauge, SlowStepUsesTheOldestKeptSecond)
{
    // At 0.5 m/s 200 m takes 400 s, longer than the history kept.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 500; ++t) {
        d += 50;
        g.tick(0.5f, true, d);
    }
    EXPECT_NEAR(g.valueMps(), 0.5f, 0.01f);
    EXPECT_EQ(g.zone(), Zone::Below);
}

TEST(SpeedGauge, WindowMatchesAReferenceAcrossTheHistoryWrap)
{
    // A varying pace over more than the history kept, checked every second
    // against the window worked out from the whole record.
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    std::vector<uint32_t> dist{0};
    for (uint32_t t = 1; t <= 900; ++t) {
        const uint32_t step = 300 + (t * 37) % 250;  // 3.0-5.5 m/s
        dist.push_back(dist.back() + step);
        g.tick(4.0f, true, dist.back());

        uint32_t from = t >= SpeedGauge::kHistory ? t - (SpeedGauge::kHistory - 1) : 0;
        for (uint32_t j = t; j-- > from;) {
            if (dist[t] - dist[j] >= SpeedGauge::kWindowCm) {
                from = j;
                break;
            }
        }
        const float window = static_cast<float>(dist[t] - dist[from]) / 100.0f
                           / static_cast<float>(t - from);
        const float w = std::exp(-static_cast<float>(t) / SpeedGauge::kBlendTauSec);
        ASSERT_NEAR(g.valueMps(), w * 4.0f + (1.0f - w) * window, 1e-4f) << "t " << t;
    }
}

TEST(SpeedGauge, NewStepForgetsTheLastStepsDistance)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 400; ++t) {
        d += 500;
        g.tick(5.0f, true, d);
    }
    g.startStep(kLow, kHigh);
    g.tick(4.0f, true, 400);
    g.tick(4.0f, true, 800);
    EXPECT_NEAR(g.stepAverageMps(), 4.0f, 1e-5f);
    EXPECT_NEAR(g.valueMps(), 4.0f, 1e-4f);  // nothing of the 5 m/s step left
}

TEST(SpeedGauge, NoSignalTicksDoNotUseUpTheHoldOff)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 20; ++t) g.tick(0.0f, false, d);
    for (int t = 1; t <= SpeedGauge::kHoldOffSec; ++t) {
        d += 426;
        g.tick(kMid, true, d);
        EXPECT_EQ(g.zone(), Zone::Unknown) << t;
    }
}

TEST(SpeedGauge, DistanceGoingBackCountsAsNoProgress)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    g.tick(4.0f, true, 4000);
    g.tick(4.0f, true, 3000);  // must not wrap to a huge distance
    EXPECT_LT(g.valueMps(), 10.0f);
    EXPECT_GE(g.valueMps(), 0.0f);
}

TEST(SpeedGauge, UntargetedStepHasNoZone)
{
    SpeedGauge g;
    g.startStep();
    uint32_t d = 0;
    for (int t = 0; t < 60; ++t) {
        d += 300;
        g.tick(3.0f, true, d);
        EXPECT_EQ(g.zone(), Zone::Unknown);
    }
    EXPECT_NEAR(g.valueMps(), 3.0f, 1e-4f);
    EXPECT_NEAR(g.stepAverageMps(), 3.0f, 1e-5f);
    g.tick(0.0f, false, d);
    EXPECT_EQ(g.zone(), Zone::NoSignal);
}

TEST(SpeedGauge, KeepsTheLastValueIntoANewStepWithoutSignal)
{
    SpeedGauge g;
    g.startStep(kLow, kHigh);
    uint32_t d = 0;
    for (int t = 0; t < 30; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    const float held = g.valueMps();
    g.startStep(kLow, kHigh);
    g.tick(0.0f, false, 0);
    EXPECT_EQ(g.zone(), Zone::NoSignal);
    EXPECT_FLOAT_EQ(g.valueMps(), held);
}

TEST(SpeedGauge, BandGivenHighFirstIsOrdered)
{
    SpeedGauge g;
    g.startStep(kHigh, kLow);
    uint32_t d = 0;
    for (int t = 0; t < 30; ++t) {
        d += 426;
        g.tick(kMid, true, d);
    }
    EXPECT_EQ(g.zone(), Zone::In);
}

// --- HeartRateGauge ------------------------------------------------------------

TEST(HeartRateGauge, ZonesWithHoldOffAndSignal)
{
    HeartRateGauge g;
    g.startStep(140.0f, 155.0f);
    for (int t = 0; t < HeartRateGauge::kHoldOffSec; ++t) {
        g.tick(148.0f, true);
        EXPECT_EQ(g.zone(), Zone::Unknown);
    }
    for (int t = 0; t < 3; ++t) g.tick(148.0f, true);
    EXPECT_EQ(g.zone(), Zone::In);
    for (int t = 0; t < 3; ++t) g.tick(157.0f, true);  // within the deadband
    EXPECT_EQ(g.zone(), Zone::In);
    for (int t = 0; t < 3; ++t) g.tick(159.0f, true);
    EXPECT_EQ(g.zone(), Zone::Above);
    g.tick(0.0f, false);
    EXPECT_EQ(g.zone(), Zone::NoSignal);
    EXPECT_FLOAT_EQ(g.valueBpm(), 159.0f);
    g.tick(130.0f, true);
    EXPECT_EQ(g.zone(), Zone::Unknown);  // held off after the gap
    for (int t = 0; t < 8; ++t) g.tick(130.0f, true);
    EXPECT_EQ(g.zone(), Zone::Below);
}

TEST(HeartRateGauge, ResumeHoldsTheZoneOff)
{
    HeartRateGauge g;
    g.startStep(140.0f, 155.0f);
    for (int t = 0; t < 8; ++t) g.tick(148.0f, true);
    ASSERT_EQ(g.zone(), Zone::In);
    g.resume();
    EXPECT_EQ(g.zone(), Zone::Unknown);
    for (int t = 0; t < HeartRateGauge::kHoldOffSec; ++t) {
        g.tick(170.0f, true);
        EXPECT_EQ(g.zone(), Zone::Unknown);
    }
    for (int t = 0; t < 3; ++t) g.tick(170.0f, true);
    EXPECT_EQ(g.zone(), Zone::Above);
}

TEST(HeartRateGauge, UntargetedStepHasNoZone)
{
    HeartRateGauge g;
    g.startStep();
    for (int t = 0; t < 20; ++t) {
        g.tick(150.0f, true);
        EXPECT_EQ(g.zone(), Zone::Unknown);
    }
    EXPECT_FLOAT_EQ(g.valueBpm(), 150.0f);
}

// --- arcFraction -----------------------------------------------------------------

TEST(ArcFraction, BandFillsTheMiddleThird)
{
    // Pace band 225-245 s/km with 30 s/km either side.
    EXPECT_FLOAT_EQ(arcFraction(225.0f, 225.0f, 245.0f, 30.0f), 1.0f / 3.0f);
    EXPECT_FLOAT_EQ(arcFraction(235.0f, 225.0f, 245.0f, 30.0f), 0.5f);
    EXPECT_FLOAT_EQ(arcFraction(245.0f, 225.0f, 245.0f, 30.0f), 2.0f / 3.0f);
    EXPECT_FLOAT_EQ(arcFraction(210.0f, 225.0f, 245.0f, 30.0f), 1.0f / 6.0f);
    EXPECT_FLOAT_EQ(arcFraction(260.0f, 225.0f, 245.0f, 30.0f), 5.0f / 6.0f);
    EXPECT_FLOAT_EQ(arcFraction(100.0f, 225.0f, 245.0f, 30.0f), 0.0f);
    EXPECT_FLOAT_EQ(arcFraction(400.0f, 225.0f, 245.0f, 30.0f), 1.0f);
    // A wider band looks the same.
    EXPECT_FLOAT_EQ(arcFraction(240.0f, 220.0f, 260.0f, 30.0f), 0.5f);
    // Band given high first.
    EXPECT_FLOAT_EQ(arcFraction(235.0f, 245.0f, 225.0f, 30.0f), 0.5f);
}

TEST(ArcFraction, EdgeCases)
{
    EXPECT_FLOAT_EQ(arcFraction(240.0f, 240.0f, 240.0f, 30.0f), 0.5f);  // zero-width band
    EXPECT_FLOAT_EQ(arcFraction(std::nanf(""), 225.0f, 245.0f, 30.0f), 0.5f);
    EXPECT_FLOAT_EQ(arcFraction(200.0f, 225.0f, 245.0f, 0.0f), 0.0f);  // no margin
}

// --- GpsCatchUp ------------------------------------------------------------------

TEST(GpsCatchUp, CaughtUpUntilAFixIsLost)
{
    GpsCatchUp c;
    EXPECT_TRUE(c.caughtUp());
    c.onLocation(true, 1000);
    c.onDistance(1500);
    EXPECT_TRUE(c.caughtUp());
    c.onLocation(false, 2000);
    EXPECT_FALSE(c.caughtUp());
}

TEST(GpsCatchUp, WaitsForTwoDistanceSamplesStampedAfterTheFix)
{
    GpsCatchUp c;
    c.onLocation(false, 1000);
    c.onDistance(1700);  // still lost: not counted
    c.onLocation(true, 2000);
    c.onDistance(1900);  // taken before the fix
    c.onDistance(2000);  // same stamp: may not have it yet
    EXPECT_FALSE(c.caughtUp());
    c.onLocation(true, 2900);  // later fixes don't move the mark
    c.onDistance(2700);
    EXPECT_FALSE(c.caughtUp());
    c.onDistance(3700);
    EXPECT_TRUE(c.caughtUp());
}

TEST(GpsCatchUp, ALossBeforeCatchingUpStartsAgain)
{
    GpsCatchUp c;
    c.onLocation(false, 1000);
    c.onLocation(true, 2000);
    c.onDistance(2700);
    c.onLocation(false, 3000);
    c.onDistance(3700);
    c.onLocation(true, 4000);
    c.onDistance(4700);
    EXPECT_FALSE(c.caughtUp());
    c.onDistance(5700);
    EXPECT_TRUE(c.caughtUp());
}

TEST(GpsCatchUp, AClockThatStartsOverDoesNotHoldItForever)
{
    GpsCatchUp c;
    c.onLocation(false, 4194303000u);
    c.onLocation(true, 4194303500u);
    c.onDistance(200);
    c.onDistance(1200);
    EXPECT_TRUE(c.caughtUp());
}

TEST(GpsCatchUp, ResetForgetsALoss)
{
    GpsCatchUp c;
    c.onLocation(false, 1000);
    c.reset();
    EXPECT_TRUE(c.caughtUp());
}

TEST(GpsCatchUp, KeepsALateCatchUpOutOfTheGauge)
{
    // In band at 4.26 m/s, with a minute without a fix mid-step. The speed
    // is valid again on the second the fix returns, but the distance across
    // the gap comes a second later. Each second: the location at +300 ms,
    // the distance at +700 ms, the gauge's tick at +900 ms.
    auto aboveTicks = [](bool hold) {
        SpeedGauge g;
        GpsCatchUp c;
        g.startStep(kLow, kHigh);
        uint32_t truth = 0, d = 0;
        int above = 0;
        for (uint32_t t = 0; t < 400; ++t) {
            const bool fix = t < 120 || t >= 180;
            truth += 426;
            const uint32_t ms = t * 1000;
            c.onLocation(fix, ms + 300);
            if (fix && t != 180) {
                d = truth;  // at 180 the distance hasn't had the fix yet
            }
            c.onDistance(ms + 700);
            g.tick(kMid, fix && (!hold || c.caughtUp()), d);
            above += g.zone() == Zone::Above;
        }
        return above;
    };
    EXPECT_GT(aboveTicks(false), 0);  // the failure the hold is for
    EXPECT_EQ(aboveTicks(true), 0);
}
