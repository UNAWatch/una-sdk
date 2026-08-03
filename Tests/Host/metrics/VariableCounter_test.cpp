/**
 * @file VariableCounter_test.cpp
 * @brief Unit tests for SDK::Metric::VariableCounter
 *
 * These pin the semantics the activity services depend on when they gate
 * add() on a valid GPS fix: what getCurrent() reports for a sample that was
 * never added, when isValid() flips, and what the maxima read when a session
 * or lap saw no valid sample at all.
 */

#include <gtest/gtest.h>

#include "SDK/Metrics/VariableCounter.hpp"

namespace {

constexpr float kMinValid = 0.5f;    // m/s, matches the activity apps
constexpr float kMaxValid = 300.0f;  // m/s

SDK::Metric::VariableCounter makeCounter()
{
    SDK::Metric::VariableCounter c;
    EXPECT_TRUE(c.init(kMinValid, kMaxValid));
    return c;
}

}  // namespace

TEST(VariableCounter, InitRejectsInvertedRange)
{
    SDK::Metric::VariableCounter c;
    EXPECT_FALSE(c.init(10.0f, 10.0f));
    EXPECT_FALSE(c.init(20.0f, 10.0f));
    EXPECT_TRUE(c.init(kMinValid, kMaxValid));
}

TEST(VariableCounter, CurrentLatchesBeforeTheRangeCheck)
{
    SDK::Metric::VariableCounter c = makeCounter();

    // An out-of-range sample is excluded from the statistics but still becomes
    // the "current" value. This is why a service that wants no out-of-range
    // data anywhere must gate add() itself rather than rely on the range.
    c.add(kMaxValid + 100.0f);

    EXPECT_FLOAT_EQ(kMaxValid + 100.0f, c.getCurrent());
    EXPECT_FALSE(c.isValid());
    EXPECT_FLOAT_EQ(0.0f, c.getMaximum());
}

TEST(VariableCounter, CurrentLatchesBeforeThePauseCheck)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(3.0f);
    c.pause();
    c.add(9.0f);

    // The paused sample does not reach the statistics...
    EXPECT_FLOAT_EQ(3.0f, c.getMaximum());
    // ...but it does become the current value. A caller that stops calling
    // add() while paused therefore freezes getCurrent(); one that keeps
    // calling it does not.
    EXPECT_FLOAT_EQ(9.0f, c.getCurrent());
}

TEST(VariableCounter, SkippedSamplesLeaveCurrentAtTheLastAdded)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(4.0f);
    // Caller decides the next samples are untrustworthy and skips them
    // entirely, as the activity services do while the GPS fix is lost.
    EXPECT_FLOAT_EQ(4.0f, c.getCurrent()) << "the last good value is held, not zeroed";
}

TEST(VariableCounter, ValidityIsStickyOnceASampleIsInRange)
{
    SDK::Metric::VariableCounter c = makeCounter();

    EXPECT_FALSE(c.isValid());

    c.add(0.1f);  // below the floor
    EXPECT_FALSE(c.isValid());

    c.add(3.0f);
    EXPECT_TRUE(c.isValid());

    // Still valid afterwards: it means "has seen a valid sample", not "the
    // latest sample is valid". Anything using it as a FIT field-presence flag
    // keeps claiming presence for the rest of the session.
    c.add(kMaxValid + 1.0f);
    EXPECT_TRUE(c.isValid());
}

TEST(VariableCounter, MaximaAreZeroWithoutAnyValidSample)
{
    SDK::Metric::VariableCounter c = makeCounter();

    // A session that never saw a valid sample reports a maximum of 0, not a
    // sentinel. A lap spent entirely without a fix therefore records
    // max_speed = 0 even though its average, computed from distance and time,
    // may be non-zero.
    EXPECT_FLOAT_EQ(0.0f, c.getMaximum());
    EXPECT_FLOAT_EQ(0.0f, c.getLapMaximum());
    EXPECT_FLOAT_EQ(0.0f, c.getMinimum());
    EXPECT_FLOAT_EQ(0.0f, c.getAverage()) << "count==0 must not divide";
    EXPECT_FALSE(c.isValid());
}

TEST(VariableCounter, TracksAverageMinAndMaxOverValidSamples)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(2.0f);
    c.add(4.0f);
    c.add(6.0f);

    EXPECT_FLOAT_EQ(4.0f, c.getAverage());
    EXPECT_FLOAT_EQ(2.0f, c.getMinimum());
    EXPECT_FLOAT_EQ(6.0f, c.getMaximum());
}

TEST(VariableCounter, OutOfRangeSamplesAreExcludedFromStatistics)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(3.0f);
    c.add(0.1f);              // below the floor
    c.add(kMaxValid + 1.0f);  // above the ceiling

    EXPECT_FLOAT_EQ(3.0f, c.getAverage()) << "only the in-range sample counts";
    EXPECT_FLOAT_EQ(3.0f, c.getMaximum());
    EXPECT_FLOAT_EQ(3.0f, c.getMinimum());
}

TEST(VariableCounter, ResetLapClearsLapStatsButNotTotalsOrCurrent)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(3.0f);
    c.add(7.0f);
    ASSERT_FLOAT_EQ(7.0f, c.getLapMaximum());

    c.resetLap();

    EXPECT_FLOAT_EQ(0.0f, c.getLapMaximum()) << "lap max restarts from nothing";
    EXPECT_FLOAT_EQ(0.0f, c.getLapAverage());
    EXPECT_FALSE(c.isLapValid());
    EXPECT_FLOAT_EQ(7.0f, c.getMaximum()) << "session max survives a lap reset";
    EXPECT_TRUE(c.isValid());
    EXPECT_FLOAT_EQ(7.0f, c.getCurrent()) << "current survives a lap reset";
}

TEST(VariableCounter, ResetClearsEverythingButKeepsTheRange)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.add(5.0f);
    c.reset();

    EXPECT_FLOAT_EQ(0.0f, c.getCurrent());
    EXPECT_FLOAT_EQ(0.0f, c.getMaximum());
    EXPECT_FALSE(c.isValid());
    EXPECT_FALSE(c.isPaused());
    EXPECT_FLOAT_EQ(kMinValid, c.getMinValid());
    EXPECT_FLOAT_EQ(kMaxValid, c.getMaxValid());
}

TEST(VariableCounter, IgnoresSamplesBeforeInit)
{
    SDK::Metric::VariableCounter c;  // no init()

    c.add(3.0f);

    EXPECT_FLOAT_EQ(0.0f, c.getCurrent());
    EXPECT_FALSE(c.isValid());
}

TEST(VariableCounter, ResumeRestoresAccumulation)
{
    SDK::Metric::VariableCounter c = makeCounter();

    c.pause();
    EXPECT_TRUE(c.isPaused());
    c.add(9.0f);
    EXPECT_FALSE(c.isValid());

    c.resume();
    EXPECT_FALSE(c.isPaused());
    c.add(4.0f);

    EXPECT_TRUE(c.isValid());
    EXPECT_FLOAT_EQ(4.0f, c.getMaximum()) << "the paused sample never counted";
}
