/**
 * Host unit tests for SensorDataParser::HeartRateEx — the opt-in multi-source
 * HR frame (arbitrated BPM/trust/source + raw optical + raw external). Lenient
 * field-count validation (a future kernel may append fields).
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"

using HeartRateEx = SDK::SensorDataParser::HeartRateEx;

namespace {

// A SDK::Sensor::Data with room for the 7 EX fields (and a spare for the
// "extra trailing field" forward-compat case).
struct HrExData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            7 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

int src(HeartRateEx::Source s) { return static_cast<int>(s); }

void fill(HrExData& m, float bpm, float trust, float source,
          float optBpm, float optTrust, float extBpm, float extTrust)
{
    m->mValue[HeartRateEx::BPM].f            = bpm;
    m->mValue[HeartRateEx::TRUST_LEVEL].f    = trust;
    m->mValue[HeartRateEx::SOURCE].f         = source;
    m->mValue[HeartRateEx::OPTICAL_BPM].f    = optBpm;
    m->mValue[HeartRateEx::OPTICAL_TRUST].f  = optTrust;
    m->mValue[HeartRateEx::EXTERNAL_BPM].f   = extBpm;
    m->mValue[HeartRateEx::EXTERNAL_TRUST].f = extTrust;
}

} // namespace

TEST(HeartRateExParser, FullFrameExternalSource)
{
    HrExData m;
    m->mTimeStamp = 1000;
    // Arbitrated == external strap; optical also present (raw).
    fill(m, 130.f, 3.f, /*source=*/2.f, /*opt=*/70.f, 2.f, /*ext=*/130.f, 3.f);

    SDK::Sensor::DataView v(*m.data(), HeartRateEx::COUNT);
    HeartRateEx p(v);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_EQ(HeartRateEx::getFieldsNumber(), 7);
    EXPECT_FLOAT_EQ(p.getBpm(), 130.f);
    EXPECT_FLOAT_EQ(p.getTrustLevel(), 3.f);
    EXPECT_EQ(src(p.getSource()), src(HeartRateEx::Source::EXTERNAL));
    EXPECT_FLOAT_EQ(p.getOpticalBpm(), 70.f);
    EXPECT_FLOAT_EQ(p.getOpticalTrust(), 2.f);
    EXPECT_FLOAT_EQ(p.getExternalBpm(), 130.f);
    EXPECT_FLOAT_EQ(p.getExternalTrust(), 3.f);
}

TEST(HeartRateExParser, OpticalSource)
{
    HrExData m;
    fill(m, 68.f, 2.f, /*source=*/1.f, /*opt=*/68.f, 2.f, /*ext=*/0.f, 0.f);

    SDK::Sensor::DataView v(*m.data(), HeartRateEx::COUNT);
    HeartRateEx p(v);

    EXPECT_EQ(src(p.getSource()), src(HeartRateEx::Source::OPTICAL));
    EXPECT_FLOAT_EQ(p.getOpticalBpm(), 68.f);
    EXPECT_FLOAT_EQ(p.getExternalBpm(), 0.f);
}

TEST(HeartRateExParser, UnknownSourceForOutOfRangeValue)
{
    HrExData m;
    fill(m, 0.f, 0.f, /*source=*/7.f, 0.f, 0.f, 0.f, 0.f);  // 7 is not a known Source

    SDK::Sensor::DataView v(*m.data(), HeartRateEx::COUNT);
    HeartRateEx p(v);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_EQ(src(p.getSource()), src(HeartRateEx::Source::UNKNOWN));
}

TEST(HeartRateExParser, ShortFrameIsInvalid)
{
    HrExData m;
    fill(m, 130.f, 3.f, 2.f, 70.f, 2.f, 130.f, 3.f);

    // Only 6 of the 7 EX fields present -> invalid; getters read zero/UNKNOWN.
    SDK::Sensor::DataView v(*m.data(), HeartRateEx::COUNT - 1);
    HeartRateEx p(v);

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
    EXPECT_FLOAT_EQ(p.getExternalBpm(), 0.f);
    EXPECT_EQ(src(p.getSource()), src(HeartRateEx::Source::UNKNOWN));
}

TEST(HeartRateExParser, ExtraTrailingFieldStillValid)
{
    HrExData m;
    fill(m, 90.f, 3.f, 2.f, 70.f, 2.f, 90.f, 3.f);

    // A future kernel appends an 8th field: lenient (>= COUNT) validation accepts it.
    SDK::Sensor::DataView v(*m.data(), HeartRateEx::COUNT + 1);
    HeartRateEx p(v);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 90.f);
    EXPECT_EQ(src(p.getSource()), src(HeartRateEx::Source::EXTERNAL));
}
