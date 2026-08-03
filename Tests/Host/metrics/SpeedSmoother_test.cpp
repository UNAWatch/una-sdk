/**
 * @file SpeedSmoother_test.cpp
 * @brief Unit tests for SDK::Metric::SpeedSmoother
 */

#include <gtest/gtest.h>

#include "SDK/Metrics/SpeedSmoother.hpp"

#include <cmath>
#include <limits>

namespace {

constexpr float kMinValid = 0.5f;    // m/s, matches the activity apps
constexpr float kMaxValid = 300.0f;  // m/s

using Smoother = SDK::Metric::SpeedSmoother<10>;

Smoother makeSmoother()
{
    Smoother s;
    EXPECT_TRUE(s.init(kMinValid, kMaxValid));
    return s;
}

/// Feed @p count valid ticks of the same speed.
void feed(Smoother &s, float speedMs, int count)
{
    for (int i = 0; i < count; i++) {
        s.tick(speedMs, true);
    }
}

}  // namespace

TEST(SpeedSmoother, InitRejectsInvertedRange)
{
    Smoother s;
    EXPECT_FALSE(s.init(10.0f, 10.0f));
    EXPECT_FALSE(s.init(20.0f, 10.0f));
    EXPECT_TRUE(s.init(0.5f, 300.0f));
}

TEST(SpeedSmoother, ReportsNothingBeforeAnySample)
{
    Smoother s = makeSmoother();

    EXPECT_FALSE(s.isValid());
    EXPECT_EQ(0u, s.getSampleCount());
    EXPECT_FLOAT_EQ(0.0f, s.getSpeed());
    EXPECT_FLOAT_EQ(0.0f, s.getPace());
}

TEST(SpeedSmoother, IgnoredWithoutInit)
{
    Smoother s;  // no init()

    s.tick(3.0f, true);

    EXPECT_FALSE(s.isValid());
    EXPECT_FLOAT_EQ(0.0f, s.getSpeed());
}

TEST(SpeedSmoother, PaceIsTheReciprocalOfTheWindowMean)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 10);  // 3 m/s == 5:33 /km

    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());
    EXPECT_FLOAT_EQ(1.0f / 3.0f, s.getPace());
}

TEST(SpeedSmoother, AveragesOverAPartiallyFilledWindow)
{
    Smoother s = makeSmoother();

    // The readout must appear immediately, averaged over what is available,
    // rather than staying blank until the window fills.
    s.tick(2.0f, true);
    EXPECT_EQ(1u, s.getSampleCount());
    EXPECT_FLOAT_EQ(2.0f, s.getSpeed());

    s.tick(4.0f, true);
    EXPECT_EQ(2u, s.getSampleCount());
    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());
}

TEST(SpeedSmoother, SuppressesSingleSampleNoise)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 10);
    ASSERT_NEAR(333.3f, s.getPace() * 1000.0f, 0.1f);  // 3 m/s == 5:33 /km

    // A 4 m/s outlier on a 3 m/s run: the raw readout would jump a full 83 s/km
    // (5:33 -> 4:10). Averaged over ten ticks it may move by about a tenth of
    // that, which is the whole point of the window.
    s.tick(4.0f, true);

    const float secPerKmSmoothed = s.getPace() * 1000.0f;
    EXPECT_NEAR(322.6f, secPerKmSmoothed, 0.1f);  // 5:23, not 4:10
}

TEST(SpeedSmoother, OldSamplesLeaveTheWindow)
{
    Smoother s = makeSmoother();

    feed(s, 2.0f, 10);
    ASSERT_FLOAT_EQ(2.0f, s.getSpeed());

    // A full window of the new speed must displace every old sample, so a
    // sustained change of effort is eventually reported in full.
    feed(s, 4.0f, 10);

    EXPECT_EQ(10u, s.getSampleCount());
    EXPECT_FLOAT_EQ(4.0f, s.getSpeed());
}

TEST(SpeedSmoother, ConvergesMonotonicallyOnAStepChange)
{
    Smoother s = makeSmoother();

    feed(s, 2.0f, 10);

    float previous = s.getSpeed();
    for (int i = 0; i < 10; i++) {
        s.tick(4.0f, true);
        const float current = s.getSpeed();
        EXPECT_GT(current, previous) << "tick " << i;
        EXPECT_LE(current, 4.0f) << "tick " << i;
        previous = current;
    }
    EXPECT_FLOAT_EQ(4.0f, previous);
}

TEST(SpeedSmoother, InvalidTicksAreSkippedButStillAgeTheWindow)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 10);
    ASSERT_EQ(10u, s.getSampleCount());

    // A dropped sample must not be read as a stop: the mean holds while the
    // sample count falls.
    s.tick(0.0f, false);
    EXPECT_EQ(9u, s.getSampleCount());
    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());

    // A lost fix for a whole window empties it, so the pace goes unavailable
    // instead of being held forward forever.
    for (int i = 0; i < 9; i++) {
        s.tick(0.0f, false);
    }
    EXPECT_EQ(0u, s.getSampleCount());
    EXPECT_FALSE(s.isValid());
    EXPECT_FLOAT_EQ(0.0f, s.getPace());
}

TEST(SpeedSmoother, DiscardsOutOfRangeSpikes)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 8);
    s.tick(kMaxValid + 1.0f, true);  // bogus chipset reading
    s.tick(-1.0f, true);             // negative speed is impossible

    // Both spikes occupy a ring slot -- they age like any other tick -- but
    // neither contributes to the mean.
    EXPECT_EQ(8u, s.getSampleCount());
    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());
}

TEST(SpeedSmoother, SlowSamplesPullTheMeanDown)
{
    Smoother s = makeSmoother();

    // Below-floor samples are real: stopping must slow the readout rather than
    // being filtered out of the average.
    feed(s, 4.0f, 5);
    feed(s, 0.0f, 5);

    EXPECT_EQ(10u, s.getSampleCount());
    EXPECT_FLOAT_EQ(2.0f, s.getSpeed());
    EXPECT_FLOAT_EQ(0.5f, s.getPace());
}

TEST(SpeedSmoother, NoPaceWhileStopped)
{
    Smoother s = makeSmoother();

    feed(s, 0.2f, 10);  // below the 0.5 m/s floor

    EXPECT_TRUE(s.isValid());
    EXPECT_FLOAT_EQ(0.2f, s.getSpeed());
    EXPECT_FLOAT_EQ(0.0f, s.getPace()) << "a stopped runner has no pace to show";
}

TEST(SpeedSmoother, NoPaceExactlyAtTheFloor)
{
    Smoother s = makeSmoother();

    feed(s, kMinValid, 10);

    EXPECT_FLOAT_EQ(0.0f, s.getPace());
}

TEST(SpeedSmoother, ResetDropsTheWindowButKeepsTheRange)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 10);
    s.reset();

    EXPECT_EQ(0u, s.getSampleCount());
    EXPECT_FLOAT_EQ(0.0f, s.getSpeed());
    EXPECT_FLOAT_EQ(kMinValid, s.getMinValid());
    EXPECT_FLOAT_EQ(kMaxValid, s.getMaxValid());

    // Still usable after a reset, with no trace of the old window.
    feed(s, 5.0f, 1);
    EXPECT_FLOAT_EQ(5.0f, s.getSpeed());
}

TEST(SpeedSmoother, ResetPartWayThroughTheRingDropsEverything)
{
    Smoother s = makeSmoother();

    // The resume-from-pause path resets mid-ring, not on a window boundary, so
    // the write cursor is left part-way along the buffer.
    feed(s, 3.0f, 3);
    s.reset();

    EXPECT_EQ(0u, s.getSampleCount());
    EXPECT_FLOAT_EQ(0.0f, s.getSpeed());

    feed(s, 5.0f, 1);
    EXPECT_EQ(1u, s.getSampleCount());
    EXPECT_FLOAT_EQ(5.0f, s.getSpeed()) << "no pre-reset sample may survive";
}

TEST(SpeedSmoother, RejectsNaNSamples)
{
    Smoother s = makeSmoother();

    feed(s, 3.0f, 5);
    s.tick(std::numeric_limits<float>::quiet_NaN(), true);

    EXPECT_EQ(5u, s.getSampleCount());
    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());
}

TEST(SpeedSmoother, WindowLengthOfOneIsPassThrough)
{
    SDK::Metric::SpeedSmoother<1> s;
    ASSERT_TRUE(s.init(kMinValid, kMaxValid));

    s.tick(3.0f, true);
    EXPECT_FLOAT_EQ(3.0f, s.getSpeed());

    s.tick(4.0f, true);
    EXPECT_FLOAT_EQ(4.0f, s.getSpeed());

    s.tick(0.0f, false);
    EXPECT_FALSE(s.isValid());
}

TEST(SpeedSmoother, LongRunDoesNotDriftTheMean)
{
    Smoother s = makeSmoother();

    // An hour of ticks: the mean is summed on demand, so it must not drift.
    feed(s, 3.3f, 3600);

    EXPECT_FLOAT_EQ(3.3f, s.getSpeed());
    EXPECT_FLOAT_EQ(1.0f / 3.3f, s.getPace());
}
