// A lap against the lap before it, by pace, and the summary's pace chart
// (LapCompare.hpp, the same in Run and RunLVGL).

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "LapCompare.hpp"

namespace {

using LapCompare::Delta;
using LapCompare::Verdict;

std::string text(const Delta& d)
{
    char buf[32];
    LapCompare::line(buf, sizeof(buf), d);
    return buf;
}

} // namespace

TEST(LapCompare, EqualLapsCompareByTime)
{
    const Delta faster = LapCompare::compare(268, 1000.0f, 271, 1000.0f);
    EXPECT_EQ(faster.verdict, Verdict::Faster);
    EXPECT_EQ(faster.s, -3);
    EXPECT_EQ(text(faster), "3 s faster");
    const Delta slower = LapCompare::compare(275, 1000.0f, 271, 1000.0f);
    EXPECT_EQ(slower.verdict, Verdict::Slower);
    EXPECT_EQ(text(slower), "4 s slower");
    const Delta even = LapCompare::compare(271, 1000.0f, 271, 1000.0f);
    EXPECT_EQ(even.verdict, Verdict::Even);
    EXPECT_EQ(text(even), "Even");
}

TEST(LapCompare, LapsOfDifferentLengthsCompareByPace)
{
    // 300 m in 81 s after 1000 m in 270 s: the same pace, 4:30 /km.
    EXPECT_EQ(LapCompare::compare(81, 300.0f, 270, 1000.0f).verdict, Verdict::Even);
    // 300 m in 78 s: 3 s faster than 4:30 /km over 300 m.
    const Delta d = LapCompare::compare(78, 300.0f, 270, 1000.0f);
    EXPECT_EQ(d.verdict, Verdict::Faster);
    EXPECT_EQ(d.s, -3);
    // Under half a second either way is even.
    EXPECT_EQ(LapCompare::compare(81, 301.0f, 270, 1000.0f).verdict, Verdict::Even);
}

TEST(LapCompare, NothingToCompare)
{
    EXPECT_EQ(LapCompare::compare(0, 1000.0f, 270, 1000.0f).verdict, Verdict::None);
    EXPECT_EQ(LapCompare::compare(270, 0.0f, 270, 1000.0f).verdict, Verdict::None);
    EXPECT_EQ(LapCompare::compare(270, 1000.0f, 0, 1000.0f).verdict, Verdict::None);
    EXPECT_EQ(LapCompare::compare(270, 1000.0f, 270, 0.0f).verdict, Verdict::None);
    // A few millimetres before a long lap: the gap is beyond any comparison.
    EXPECT_EQ(LapCompare::compare(300, 1000.0f, 3600, 0.001f).verdict, Verdict::None);
    EXPECT_EQ(text(Delta()), "");
}

TEST(LapCompare, AMinuteOrMoreReadsAsMinutes)
{
    EXPECT_EQ(text(LapCompare::compare(330, 1000.0f, 268, 1000.0f)), "1:02 slower");
}

TEST(LapCompare, TheTrackerComparesOnlyWithTheLapJustBefore)
{
    LapCompare::Tracker t;
    EXPECT_EQ(t.add(0, 270, 1000.0f).verdict, Verdict::None) << "the first lap";
    EXPECT_EQ(t.add(1, 266, 1000.0f).verdict, Verdict::Faster);
    EXPECT_EQ(t.last().s, -4);
    // Lap 3 went by unseen: lap 4 has nothing to compare with.
    EXPECT_EQ(t.add(3, 280, 1000.0f).verdict, Verdict::None);
    EXPECT_EQ(t.add(4, 281, 1000.0f).verdict, Verdict::Slower);
    t.reset();
    EXPECT_EQ(t.add(5, 270, 1000.0f).verdict, Verdict::None) << "a new run";
}

TEST(LapCompare, TheSameLapAgainIsStillComparedWithTheLapBefore)
{
    // A Lap press: the GUI takes the lap at the press, then again when the
    // service ends it, with its final figures.
    LapCompare::Tracker t;
    t.add(0, 270, 1000.0f);
    EXPECT_EQ(t.add(1, 265, 1000.0f).verdict, Verdict::Faster);
    const Delta again = t.add(1, 266, 1000.0f);
    EXPECT_EQ(again.verdict, Verdict::Faster) << "not cleared by the repeat";
    EXPECT_EQ(again.s, -4) << "the later figures";
    EXPECT_EQ(t.add(2, 270, 1000.0f).s, 4) << "the next lap against the final figures";
}

TEST(LapCompare, TheChartsFastestAndSlowestLeaveOutLapsTooShortToCount)
{
    struct Lap {
        float pace;   // s/m
        float m;
    };
    // 4:30, 4:10, 4:50 /km, then a 40 m last lap at 3:00 /km.
    const std::vector<Lap> laps = {{0.270f, 1000.0f}, {0.250f, 1000.0f}, {0.290f, 1000.0f}, {0.180f, 40.0f}};
    const auto e = LapCompare::extremes(laps.size(), [&](size_t i) { return laps[i].pace; },
                                        [&](size_t i) { return laps[i].m; });
    EXPECT_EQ(e.fastest, 1);
    EXPECT_EQ(e.slowest, 2);
    // One lap long enough: nothing to mark.
    const std::vector<Lap> one = {{0.270f, 1000.0f}, {0.180f, 40.0f}};
    const auto f = LapCompare::extremes(one.size(), [&](size_t i) { return one[i].pace; },
                                        [&](size_t i) { return one[i].m; });
    EXPECT_EQ(f.fastest, -1);
    EXPECT_EQ(f.slowest, -1);
}
