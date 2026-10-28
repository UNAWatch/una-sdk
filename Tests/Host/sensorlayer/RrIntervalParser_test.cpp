/**
 * Host unit tests for SensorDataParser::RrInterval, the opt-in beat-to-beat
 * R-R interval frame (one interval per frame, u32 source and flags, lenient
 * field-count validation like HeartRateEx).
 */

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserRrInterval.hpp"

using RrInterval = SDK::SensorDataParser::RrInterval;

namespace {

struct RrData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            3 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

int src(RrInterval::Source s) { return static_cast<int>(s); }

RrInterval parse(RrData& m, uint16_t fields = RrInterval::COUNT)
{
    return RrInterval(SDK::Sensor::DataView(*m.data(), fields));
}

} // namespace

TEST(RrIntervalParser, WireNumbersAreFrozen)
{
    EXPECT_EQ(static_cast<uint32_t>(SDK::Sensor::Type::RR_INTERVAL), 0x44u);

    EXPECT_EQ(RrInterval::RR_MS,  0);
    EXPECT_EQ(RrInterval::SOURCE, 1);
    EXPECT_EQ(RrInterval::FLAGS,  2);
    EXPECT_EQ(RrInterval::COUNT,  3);
    EXPECT_EQ(RrInterval::getFieldsNumber(), 3);

    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::UNKNOWN),  0u);
    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::OPTICAL),  1u);
    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::EXTERNAL), 2u);

    EXPECT_EQ(RrInterval::Flag::DISCONTINUITY,    1u << 0);
    EXPECT_EQ(RrInterval::Flag::ARTIFACT_SUSPECT, 1u << 1);
}

TEST(RrIntervalParser, ParsesAFrameWrittenWithRawWireValues)
{
    RrData m;
    m->mTimeStamp    = 1000;
    m->mTimeStampUs  = 469;
    m->mValue[0].f   = 855.f;
    m->mValue[1].u32 = 2u;
    m->mValue[2].u32 = 1u;

    RrInterval p = parse(m);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 855.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::EXTERNAL));
    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
    EXPECT_EQ(p.getTimestamp(), 1000u);
    EXPECT_EQ(p.getTimestampUs(), 1000469ull);
}

TEST(RrIntervalParser, OpticalArtifactSuspect)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 812.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::OPTICAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::ARTIFACT_SUSPECT;

    RrInterval p = parse(m);

    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::OPTICAL));
    EXPECT_TRUE(p.isArtifactSuspect());
    EXPECT_FALSE(p.hasDiscontinuity());
}

TEST(RrIntervalParser, FlagsCombineAndUndefinedBitsAreIgnored)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f   = 850.f;
    m->mValue[RrInterval::FLAGS].u32 = RrInterval::Flag::DISCONTINUITY |
                                       RrInterval::Flag::ARTIFACT_SUSPECT | (1u << 24);

    RrInterval p = parse(m);

    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_TRUE(p.isArtifactSuspect());
}

TEST(RrIntervalParser, ZeroMetadataReadsAsNotReported)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f = 855.f;

    RrInterval p = parse(m);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
}

TEST(RrIntervalParser, UnknownSourceForOutOfRangeValue)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 700.f;
    m->mValue[RrInterval::SOURCE].u32 = 9u;

    EXPECT_EQ(src(parse(m).getSource()), src(RrInterval::Source::UNKNOWN));
}

TEST(RrIntervalParser, ShortFrameIsInvalid)
{
    RrData m;
    m->mTimeStamp                     = 1000;
    m->mValue[RrInterval::RR_MS].f    = 855.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY;

    RrInterval p = parse(m, RrInterval::COUNT - 1);

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 0.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_EQ(p.getTimestamp(), 0u);
    EXPECT_EQ(p.getTimestampUs(), 0u);
}

TEST(RrIntervalParser, ExtraTrailingFieldStillValid)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 855.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::COUNT].u32  = 0xDEADBEEFu;

    RrInterval p = parse(m, RrInterval::COUNT + 1);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 855.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::EXTERNAL));
}

TEST(RrIntervalParser, MalformedIntervalInvalidatesTheWholeFrame)
{
    const float bad[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        0.f,
        -800.f,
    };

    for (float v : bad) {
        RrData m;
        m->mTimeStamp                     = 1000;
        m->mValue[RrInterval::RR_MS].f    = v;
        m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
        m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY |
                                            RrInterval::Flag::ARTIFACT_SUSPECT;

        RrInterval p = parse(m);

        EXPECT_FALSE(p.isDataValid()) << v;
        EXPECT_FLOAT_EQ(p.getRrMs(), 0.f);
        EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
        EXPECT_FALSE(p.hasDiscontinuity());
        EXPECT_FALSE(p.isArtifactSuspect());
        EXPECT_EQ(p.getTimestamp(), 0u);
        EXPECT_EQ(p.getTimestampUs(), 0u);
    }
}

TEST(RrIntervalParser, ImplausibleButFiniteIntervalsArePassedThrough)
{
    const float values[] = { 5.f, 40000.f };

    for (float v : values) {
        RrData m;
        m->mValue[RrInterval::RR_MS].f = v;

        RrInterval p = parse(m);

        EXPECT_TRUE(p.isDataValid());
        EXPECT_FLOAT_EQ(p.getRrMs(), v);
    }
}
