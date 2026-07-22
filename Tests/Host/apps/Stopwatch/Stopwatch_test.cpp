#include <gtest/gtest.h>

#include "Stopwatch.hpp"

using Stopwatch::Core;
using Stopwatch::elapsed;
using Stopwatch::lapDuration;

TEST(StopwatchCore, StartsCleared)
{
    Core sw;

    EXPECT_FALSE(sw.isRunning());
    EXPECT_EQ(sw.state().lapCount, 0);
    EXPECT_EQ(elapsed(sw.state(), 12345), 0u);
}

TEST(StopwatchCore, PausedClockDoesNotFollowTheTick)
{
    Core sw;

    sw.start(1000);
    EXPECT_EQ(elapsed(sw.state(), 1500), 500u);

    sw.pause(1500);
    EXPECT_EQ(elapsed(sw.state(), 9999), 500u);
}

TEST(StopwatchCore, ResumeBanksTheEarlierRun)
{
    Core sw;

    sw.start(1000);
    sw.pause(1500);
    sw.start(2000);

    EXPECT_EQ(elapsed(sw.state(), 2300), 800u);
}

TEST(StopwatchCore, RedundantStartKeepsTheOriginalBaseline)
{
    Core sw;

    sw.start(1000);
    sw.start(5000);

    EXPECT_EQ(elapsed(sw.state(), 1500), 500u);
}

TEST(StopwatchCore, RedundantPauseBanksNothingExtra)
{
    Core sw;

    sw.start(1000);
    sw.pause(1500);
    sw.pause(8000);

    EXPECT_EQ(elapsed(sw.state(), 9000), 500u);
}

TEST(StopwatchCore, LapsStoreBoundariesAndDerivedDurationsSumToTheTotal)
{
    Core sw;
    sw.start(0);

    sw.lap(8160);
    sw.lap(23270);

    const Stopwatch::State &s = sw.state();
    ASSERT_EQ(s.lapCount, 2);
    EXPECT_EQ(s.laps[0], 8160u);
    EXPECT_EQ(s.laps[1], 23270u);

    EXPECT_EQ(lapDuration(s, 0), 8160u);
    EXPECT_EQ(lapDuration(s, 1), 15110u);
    EXPECT_EQ(lapDuration(s, 0) + lapDuration(s, 1), elapsed(s, 23270));
}

TEST(StopwatchCore, LapDurationOutOfRangeIsZero)
{
    Core sw;
    sw.start(0);
    sw.lap(100);

    EXPECT_EQ(lapDuration(sw.state(), 1), 0u);
}

TEST(StopwatchCore, LapIsRefusedWhenNotRunning)
{
    Core sw;

    EXPECT_FALSE(sw.lap(1000));

    sw.start(0);
    sw.pause(500);
    EXPECT_FALSE(sw.lap(600));
    EXPECT_EQ(sw.state().lapCount, 0);
}

TEST(StopwatchCore, LapStoreIsBounded)
{
    Core sw;
    sw.start(0);

    for (size_t i = 0; i < Stopwatch::kMaxLaps; ++i) {
        ASSERT_TRUE(sw.lap(static_cast<uint32_t>((i + 1) * 100)));
    }
    EXPECT_EQ(sw.state().lapCount, Stopwatch::kMaxLaps);

    EXPECT_FALSE(sw.lap(999999));
    EXPECT_EQ(sw.state().lapCount, Stopwatch::kMaxLaps);
}

TEST(StopwatchCore, ResetClearsEverything)
{
    Core sw;
    sw.start(0);
    sw.lap(100);
    sw.pause(200);

    sw.reset();

    EXPECT_FALSE(sw.isRunning());
    EXPECT_EQ(sw.state().lapCount, 0);
    EXPECT_EQ(elapsed(sw.state(), 5000), 0u);
}

// The service keys its lifetime on isRunning(): once the GUI is gone it stays
// resident only while the clock advances. A stopped clock reports not-running
// whether it still holds banked time or laps, so in every stopped shape the
// service is free to exit.
TEST(StopwatchCore, StoppedClockIsNotRunningWhateverItHolds)
{
    Core withTime;
    withTime.start(0);
    withTime.pause(1);
    EXPECT_FALSE(withTime.isRunning());

    Core withLaps;
    withLaps.start(0);
    withLaps.lap(100);
    withLaps.pause(100);
    EXPECT_FALSE(withLaps.isRunning());
}

// The kernel tick is a uint32_t of milliseconds, so it wraps about every
// 49.7 days and a run can straddle the wrap.
TEST(StopwatchCore, SurvivesTheTickWrap)
{
    Core sw;
    const uint32_t start = 0xFFFFFFFF - 999;

    sw.start(start);
    EXPECT_EQ(elapsed(sw.state(), 500), 1500u);

    sw.lap(500);
    EXPECT_EQ(lapDuration(sw.state(), 0), 1500u);

    sw.pause(500);
    EXPECT_EQ(elapsed(sw.state(), 12345), 1500u);
}
