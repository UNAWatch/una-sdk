/**
 ******************************************************************************
 * @file    ClockTime_test.cpp
 * @brief   Tests for the SDK::Clock time-of-day helpers.
 ******************************************************************************
 */

#include "SDK/Utils/ClockTime.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using SDK::Clock::to12Hour;
using SDK::Clock::to24Hour;

TEST(ClockTime, To12HourBoundaries)
{
    EXPECT_EQ(to12Hour(0).hour, 12u);   EXPECT_FALSE(to12Hour(0).pm);   // midnight -> 12 AM
    EXPECT_EQ(to12Hour(1).hour, 1u);    EXPECT_FALSE(to12Hour(1).pm);
    EXPECT_EQ(to12Hour(11).hour, 11u);  EXPECT_FALSE(to12Hour(11).pm);
    EXPECT_EQ(to12Hour(12).hour, 12u);  EXPECT_TRUE(to12Hour(12).pm);   // noon -> 12 PM
    EXPECT_EQ(to12Hour(13).hour, 1u);   EXPECT_TRUE(to12Hour(13).pm);
    EXPECT_EQ(to12Hour(23).hour, 11u);  EXPECT_TRUE(to12Hour(23).pm);
}

TEST(ClockTime, To24HourInverts)
{
    EXPECT_EQ(to24Hour(12, false), 0u);   // 12 AM -> 00
    EXPECT_EQ(to24Hour(12, true), 12u);   // 12 PM -> 12
    EXPECT_EQ(to24Hour(1, false), 1u);
    EXPECT_EQ(to24Hour(11, true), 23u);
}

TEST(ClockTime, RoundTripEveryHour)
{
    for (int h = 0; h < 24; ++h) {
        const auto t = to12Hour(static_cast<std::uint8_t>(h));
        EXPECT_GE(t.hour, 1u);
        EXPECT_LE(t.hour, 12u);
        EXPECT_EQ(to24Hour(t.hour, t.pm), static_cast<std::uint8_t>(h))
            << "round-trip failed at hour " << h;
    }
}

// Usable in constant expressions.
TEST(ClockTime, ConstexprEvaluable)
{
    static_assert(to12Hour(0).hour == 12 && !to12Hour(0).pm, "midnight");
    static_assert(to12Hour(12).hour == 12 && to12Hour(12).pm, "noon");
    static_assert(to12Hour(13).hour == 1 && to12Hour(13).pm, "1 PM");
    static_assert(to24Hour(12, false) == 0, "12 AM -> 0");
    static_assert(to24Hour(12, true) == 12, "12 PM -> 12");
    SUCCEED();
}
