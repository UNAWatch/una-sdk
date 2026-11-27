// Oval::LaneTable: lap lengths and line offsets.

#include "Oval/LaneTable.hpp"

#include <gtest/gtest.h>

#include <cmath>

using Oval::LaneTable;

TEST(LaneTable, OutdoorLengthsMatchTheStandard)
{
    const LaneTable t = LaneTable::outdoor();
    const float expected[8] = {400.00f, 407.04f, 414.70f, 422.37f, 430.03f, 437.70f, 445.37f, 453.03f};
    ASSERT_EQ(t.laneCount(), 8);
    for (uint8_t lane = 1; lane <= 8; ++lane) {
        EXPECT_NEAR(t.lapLengthM(lane), expected[lane - 1], 0.01f) << "lane " << int(lane);
    }
}

TEST(LaneTable, OutdoorLineOffsets)
{
    const LaneTable t = LaneTable::outdoor();
    const float expected[8] = {0.30f, 1.42f, 2.64f, 3.86f, 5.08f, 6.30f, 7.52f, 8.74f};
    for (uint8_t lane = 1; lane <= 8; ++lane) {
        EXPECT_NEAR(t.lineOffsetM(lane), expected[lane - 1], 1e-5f) << "lane " << int(lane);
    }
}

// The table holds for any single-radius 400 m track: straights shrink as the
// bends grow, so only the bends' extra length per lane matters.
TEST(LaneTable, OutdoorHoldsForAnyBendRadius)
{
    const LaneTable t  = LaneTable::outdoor();
    const double    pi = 3.141592653589793;
    for (double kerbR = 35.0; kerbR <= 38.0; kerbR += 0.5) {
        const double straight = (400.0 - 2.0 * pi * (kerbR + 0.30)) / 2.0;
        for (uint8_t lane = 1; lane <= 8; ++lane) {
            const double geometric = 2.0 * straight + 2.0 * pi * (kerbR + t.lineOffsetM(lane));
            EXPECT_NEAR(t.lapLengthM(lane), geometric, 1e-3) << "radius " << kerbR << " lane " << int(lane);
        }
    }
}

TEST(LaneTable, InvalidLanes)
{
    const LaneTable t = LaneTable::outdoor();
    EXPECT_FALSE(t.isValidLane(0));
    EXPECT_FALSE(t.isValidLane(9));
    EXPECT_EQ(t.lapLengthM(0), 0.0f);
    EXPECT_EQ(t.lapLengthM(9), 0.0f);
    EXPECT_EQ(t.lineOffsetM(9), 0.0f);
}

TEST(LaneTable, DefaultIsOutdoor)
{
    const LaneTable t;
    EXPECT_EQ(t.laneCount(), 8);
    EXPECT_FLOAT_EQ(t.lapLengthM(1), 400.0f);
}

