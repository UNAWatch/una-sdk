/**
 * @file DeltaCounter_test.cpp
 * @brief Unit tests for SDK::Metric::DeltaCounter
 *
 * The accumulator behind every activity app's elevation gain, fed ~1 Hz filtered
 * barometric altitude with a 2 m threshold.
 */

#include <gtest/gtest.h>

#include "SDK/Metrics/DeltaCounter.hpp"

namespace {

constexpr float kMinChange = 2.0f;   // m, matches all four activity apps

SDK::Metric::DeltaCounter makeCounter()
{
    SDK::Metric::DeltaCounter c;
    EXPECT_TRUE(c.init(kMinChange));
    return c;
}

}  // namespace

TEST(DeltaCounter, InitRejectsNegativeThreshold)
{
    SDK::Metric::DeltaCounter c;
    EXPECT_FALSE(c.init(-0.1f));
    EXPECT_TRUE(c.init(0.0f));
    EXPECT_TRUE(c.init(kMinChange));
}

TEST(DeltaCounter, AddIsIgnoredBeforeInit)
{
    SDK::Metric::DeltaCounter c;    // no init()
    c.add(700.0f);
    EXPECT_FALSE(c.isValid());
    EXPECT_FLOAT_EQ(0.0f, c.getCurrent());
}

TEST(DeltaCounter, FirstSampleOnlySeedsTheBaseline)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    EXPECT_FALSE(c.isValid());

    c.add(700.0f);   // switching on at 700 m is not a 700 m climb

    EXPECT_TRUE(c.isValid());
    EXPECT_FLOAT_EQ(700.0f, c.getCurrent());
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getDescent());
    EXPECT_FLOAT_EQ(0.0f, c.getDelta());
}

TEST(DeltaCounter, SubThresholdDriftIsHeldNotDiscarded)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);

    // A slow climb arrives in sub-threshold steps; it must be held, not dropped.
    c.add(101.0f);
    c.add(101.5f);
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());

    c.add(102.0f);
    EXPECT_FLOAT_EQ(2.0f, c.getAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getDescent());
}

TEST(DeltaCounter, SteadyClimbSumsToTheClimb)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(0.0f);
    for (int i = 1; i <= 100; ++i) {
        c.add(static_cast<float>(i));
    }
    EXPECT_NEAR(100.0f, c.getAscent(), 0.01f);
    EXPECT_FLOAT_EQ(0.0f, c.getDescent());
    EXPECT_NEAR(100.0f, c.getDelta(), 0.01f);
}

TEST(DeltaCounter, DescentIsReportedPositive)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(500.0f);
    c.add(490.0f);

    EXPECT_FLOAT_EQ(10.0f, c.getDescent());
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
    EXPECT_FLOAT_EQ(-10.0f, c.getDelta());
}

TEST(DeltaCounter, NoiseBelowThresholdNeverAccumulates)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);

    for (int i = 0; i < 500; ++i) {
        c.add(100.0f + ((i % 2 == 0) ? 1.9f : -1.9f));
    }
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getDescent());
}

TEST(DeltaCounter, NoiseAboveThresholdDoesAccumulate)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);

    // Deliberate: past the threshold, noise is indistinguishable from terrain. No
    // accumulator topology avoids this -- only filtering, or a higher threshold.
    for (int i = 0; i < 10; ++i) {
        c.add(100.0f + ((i % 2 == 0) ? 2.5f : -2.5f));
    }
    EXPECT_GT(c.getAscent(), 0.0f);
    EXPECT_GT(c.getDescent(), 0.0f);
    EXPECT_NEAR(0.0f, c.getDelta(), 2.6f);   // net stays ~zero, so delta is safe
}

TEST(DeltaCounter, PauseStopsAccumulation)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);
    c.add(110.0f);
    EXPECT_FLOAT_EQ(10.0f, c.getAscent());

    c.pause();
    EXPECT_TRUE(c.isPaused());
    c.add(120.0f);
    c.add(130.0f);

    EXPECT_FLOAT_EQ(10.0f, c.getAscent());
    EXPECT_FLOAT_EQ(130.0f, c.getCurrent());
    EXPECT_FLOAT_EQ(30.0f, c.getDelta());   // delta ignores pause by design
}

TEST(DeltaCounter, ResumeDoesNotCreditThePauseWindow)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);

    // Auto-paused at the foot of a hill, then carried up it before resuming.
    c.pause();
    c.add(150.0f);
    c.resume();
    c.add(150.0f);

    EXPECT_FLOAT_EQ(0.0f, c.getAscent());   // read 50 before resume() rebased

    c.add(153.0f);
    EXPECT_FLOAT_EQ(3.0f, c.getAscent());
}

TEST(DeltaCounter, ResumeDoesNotCreditThePauseWindowToTheLap)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);

    c.pause();
    c.add(150.0f);
    c.resume();
    c.add(150.0f);

    EXPECT_FLOAT_EQ(0.0f, c.getLapAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getLapDescent());
}

TEST(DeltaCounter, ResumeWithoutDataIsHarmless)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.pause();
    c.resume();

    c.add(700.0f);
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
    c.add(702.0f);
    EXPECT_FLOAT_EQ(2.0f, c.getAscent());
}

TEST(DeltaCounter, ResetLapKeepsTotalsAndRebasesTheLap)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);
    c.add(110.0f);
    EXPECT_FLOAT_EQ(10.0f, c.getLapAscent());

    c.resetLap();
    EXPECT_FALSE(c.isLapValid());
    EXPECT_FLOAT_EQ(10.0f, c.getAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getLapAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getLapDelta());

    c.add(115.0f);
    EXPECT_TRUE(c.isLapValid());              // stayed false from lap 2 onwards
    EXPECT_FLOAT_EQ(15.0f, c.getAscent());
    EXPECT_FLOAT_EQ(5.0f, c.getLapAscent());
    EXPECT_FLOAT_EQ(5.0f, c.getLapDelta());
}

TEST(DeltaCounter, ResetClearsEverything)
{
    SDK::Metric::DeltaCounter c = makeCounter();
    c.add(100.0f);
    c.add(120.0f);
    c.pause();

    c.reset();

    EXPECT_FALSE(c.isValid());
    EXPECT_FALSE(c.isPaused());
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
    EXPECT_FLOAT_EQ(0.0f, c.getDescent());
    EXPECT_FLOAT_EQ(0.0f, c.getDelta());
    EXPECT_FLOAT_EQ(0.0f, c.getCurrent());

    c.add(700.0f);
    EXPECT_FLOAT_EQ(0.0f, c.getAscent());
}
