/**
 * Tests for the numeric getters of SDK::JsonStreamReader: integer, boolean
 * (0/1), 64-bit and floating-point values read from a JSON buffer.
 *
 * The buffer is not NUL-terminated after each value, so these also check that
 * a number ends at the following ',' or '}'.
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"

namespace {

const char kJson[] =
    "{\"pos\":42,\"neg\":-7,\"zero\":0,"
    "\"one\":1,\"two\":2,\"t\":true,"
    "\"big\":9007199254740993,\"negbig\":-9007199254740993,"
    "\"half\":1.5,\"frac\":-0.125,\"exp\":1e3,\"int_as_float\":1000,"
    "\"arr\":[3,4.25]}";

class JsonStreamReaderNumbers : public ::testing::Test {
protected:
    void SetUp() override
    {
        reader.loadBuffer(kJson, std::strlen(kJson));
        ASSERT_TRUE(reader.validate());
    }

    SDK::JsonStreamReader reader;
};

TEST_F(JsonStreamReaderNumbers, Int32)
{
    int32_t v = 0;
    ASSERT_TRUE(reader.get("pos", v));
    EXPECT_EQ(v, 42);
    ASSERT_TRUE(reader.get("neg", v));
    EXPECT_EQ(v, -7);
    ASSERT_TRUE(reader.get("zero", v));
    EXPECT_EQ(v, 0);
    ASSERT_TRUE(reader.get("arr[0]", v));
    EXPECT_EQ(v, 3);
}

TEST_F(JsonStreamReaderNumbers, NarrowIntegersGoThroughInt32)
{
    int8_t i8 = 0;
    ASSERT_TRUE(reader.get("neg", i8));
    EXPECT_EQ(i8, -7);
    int16_t i16 = 0;
    ASSERT_TRUE(reader.get("pos", i16));
    EXPECT_EQ(i16, 42);
}

TEST_F(JsonStreamReaderNumbers, BoolFromNumberAcceptsOnlyZeroOrOne)
{
    bool b = false;
    ASSERT_TRUE(reader.get("one", b));
    EXPECT_TRUE(b);
    ASSERT_TRUE(reader.get("zero", b));
    EXPECT_FALSE(b);
    EXPECT_FALSE(reader.get("two", b));
    EXPECT_FALSE(reader.get("neg", b));
    ASSERT_TRUE(reader.get("t", b));
    EXPECT_TRUE(b);
}

TEST_F(JsonStreamReaderNumbers, Int64KeepsAllDigits)
{
    int64_t v = 0;
    ASSERT_TRUE(reader.get("big", v));
    EXPECT_EQ(v, INT64_C(9007199254740993));
    ASSERT_TRUE(reader.get("negbig", v));
    EXPECT_EQ(v, INT64_C(-9007199254740993));
}

TEST_F(JsonStreamReaderNumbers, Float)
{
    float v = 0.0f;
    ASSERT_TRUE(reader.get("half", v));
    EXPECT_FLOAT_EQ(v, 1.5f);
    ASSERT_TRUE(reader.get("exp", v));
    EXPECT_FLOAT_EQ(v, 1000.0f);
    ASSERT_TRUE(reader.get("int_as_float", v));
    EXPECT_FLOAT_EQ(v, 1000.0f);
    ASSERT_TRUE(reader.get("arr[1]", v));
    EXPECT_FLOAT_EQ(v, 4.25f);
}

TEST_F(JsonStreamReaderNumbers, Double)
{
    double v = 0.0;
    ASSERT_TRUE(reader.get("frac", v));
    EXPECT_DOUBLE_EQ(v, -0.125);
    ASSERT_TRUE(reader.get("half", v));
    EXPECT_DOUBLE_EQ(v, 1.5);
}

TEST_F(JsonStreamReaderNumbers, MissingKeyLeavesValueUntouched)
{
    int32_t i = 99;
    EXPECT_FALSE(reader.get("absent", i));
    EXPECT_EQ(i, 99);
    double d = 2.5;
    EXPECT_FALSE(reader.get("absent", d));
    EXPECT_DOUBLE_EQ(d, 2.5);
}

} // namespace
