/**
 * Tests for SDK::ParseVersion -- turns a "major.minor.patch" build version
 * string into a FirmwareVersion. A string without all three numeric fields
 * yields 0.0.0; anything after the patch number (a pre-release suffix) is
 * ignored.
 */
#include <gtest/gtest.h>

#include "SDK/Tools/FirmwareVersion.hpp"

namespace {

void expectVersion(const char* str, int major, int minor, int patch)
{
    SCOPED_TRACE(str ? str : "(null)");
    const SDK::FirmwareVersion v = SDK::ParseVersion(str);
    EXPECT_EQ(v.major, major);
    EXPECT_EQ(v.minor, minor);
    EXPECT_EQ(v.patch, patch);
}

TEST(FirmwareVersion, ParsesThreeFields)
{
    expectVersion("1.7.0", 1, 7, 0);
    expectVersion("12.34.56", 12, 34, 56);
    expectVersion("0.0.1", 0, 0, 1);
}

TEST(FirmwareVersion, IgnoresSuffixAfterPatch)
{
    expectVersion("1.6.0-rc2", 1, 6, 0);
    expectVersion("1.0.0-dirty", 1, 0, 0);
    expectVersion("2.3.4.5", 2, 3, 4);
}

TEST(FirmwareVersion, SkipsLeadingWhitespace)
{
    expectVersion(" 1.2.3", 1, 2, 3);
}

TEST(FirmwareVersion, IncompleteVersionIsZero)
{
    expectVersion(nullptr, 0, 0, 0);
    expectVersion("", 0, 0, 0);
    expectVersion("abc", 0, 0, 0);
    expectVersion("1", 0, 0, 0);
    expectVersion("1.2", 0, 0, 0);
    expectVersion("1.2.", 0, 0, 0);
    expectVersion("1..3", 0, 0, 0);
    expectVersion(".2.3", 0, 0, 0);
    expectVersion("1.2.x", 0, 0, 0);
}

} // namespace
