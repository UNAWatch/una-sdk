// Unit tests for SDK::Utils::formatTimeOfDay -- the single source of truth for
// how the status-face clock is rendered across all apps.
//
// The behaviour that most needs pinning down: Hour24 is zero-padded so it stays
// distinct from Hour12 for morning hours (09:05 vs 9:05). An unpadded 24-hour
// reading would be byte-identical to the 12-hour one for every hour 1..12,
// making the setting invisible for half the day -- the regression this guards.

#include <gtest/gtest.h>

#include "SDK/Utils/TimeFormat.hpp"

#include <string>

using SDK::Message::TimeFormat;

namespace {

std::string fmt(std::uint8_t h, std::uint8_t m, TimeFormat f)
{
    char buf[8];
    SDK::Utils::formatTimeOfDay(buf, sizeof(buf), h, m, f);
    return std::string(buf);
}

} // namespace

TEST(FormatTimeOfDay, Hour24IsZeroPadded)
{
    EXPECT_EQ(fmt(0, 0, TimeFormat::Hour24), "00:00");
    EXPECT_EQ(fmt(9, 5, TimeFormat::Hour24), "09:05");
    EXPECT_EQ(fmt(17, 42, TimeFormat::Hour24), "17:42");
    EXPECT_EQ(fmt(23, 59, TimeFormat::Hour24), "23:59");
}

TEST(FormatTimeOfDay, Hour12HasNoLeadingZeroAndNoMeridiem)
{
    // Noon and midnight both display as 12, not 0.
    EXPECT_EQ(fmt(0, 0, TimeFormat::Hour12), "12:00");   // midnight
    EXPECT_EQ(fmt(12, 0, TimeFormat::Hour12), "12:00");  // noon
    EXPECT_EQ(fmt(13, 0, TimeFormat::Hour12), "1:00");
    EXPECT_EQ(fmt(9, 5, TimeFormat::Hour12), "9:05");
    EXPECT_EQ(fmt(17, 42, TimeFormat::Hour12), "5:42");
    EXPECT_EQ(fmt(23, 59, TimeFormat::Hour12), "11:59");
}

TEST(FormatTimeOfDay, Hour24CompactIsZeroPaddedWithNoSeparator)
{
    EXPECT_EQ(fmt(0, 0, TimeFormat::Hour24Compact), "0000");
    EXPECT_EQ(fmt(9, 5, TimeFormat::Hour24Compact), "0905");
    EXPECT_EQ(fmt(17, 42, TimeFormat::Hour24Compact), "1742");
    EXPECT_EQ(fmt(23, 59, TimeFormat::Hour24Compact), "2359");
}

// The core reason Hour24 is zero-padded: without it, the 12/24 options are
// indistinguishable for single-digit morning hours (would both be "9:05").
// Hours 10-12 unavoidably coincide -- only an AM/PM marker could separate
// them, and the status face has no such glyphs.
TEST(FormatTimeOfDay, Hour24AndHour12DifferForSingleDigitMorningHours)
{
    for (std::uint8_t h = 1; h <= 9; ++h) {
        EXPECT_NE(fmt(h, 5, TimeFormat::Hour24), fmt(h, 5, TimeFormat::Hour12))
            << "hour " << static_cast<int>(h) << " renders identically in 24h and 12h";
    }
    // Documented coincidence for the two-digit morning hours.
    for (std::uint8_t h = 10; h <= 12; ++h) {
        EXPECT_EQ(fmt(h, 5, TimeFormat::Hour24), fmt(h, 5, TimeFormat::Hour12));
    }
}

// An out-of-range/unknown enum value degrades gracefully to Hour24 rather than
// producing garbage -- matches the switch default the call sites rely on.
TEST(FormatTimeOfDay, UnknownFormatFallsBackToHour24)
{
    const auto bogus = static_cast<TimeFormat>(99);
    EXPECT_EQ(fmt(9, 5, bogus), "09:05");
    EXPECT_EQ(fmt(17, 42, bogus), "17:42");
}
