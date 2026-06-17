/**
 * Host unit tests for SensorDataParser::HeartRate — the WP-S1 changes:
 * lenient field-count validation (forward/backward compatible) and the optional
 * source field (getSource).
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"

using HeartRate = SDK::SensorDataParser::HeartRate;

namespace {

// A SDK::Sensor::Data with room for up to 4 float fields.
struct HrData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            3 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

int src(HeartRate::Source s) { return static_cast<int>(s); }

} // namespace

TEST(HeartRateParser, TwoFieldFrameValidSourceUnknown)
{
    HrData m;
    m->mTimeStamp = 1000;
    m->mValue[HeartRate::BPM].f = 72.f;
    m->mValue[HeartRate::TRUST_LEVEL].f = 3.f;

    SDK::Sensor::DataView v(*m.data(), 2);
    HeartRate p(v);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 72.f);
    EXPECT_FLOAT_EQ(p.getTrustLevel(), 3.f);
    // No third field -> source unknown; base layout count unchanged.
    EXPECT_EQ(src(p.getSource()), src(HeartRate::Source::UNKNOWN));
    EXPECT_EQ(HeartRate::getFieldsNumber(), 2);
}

TEST(HeartRateParser, ThreeFieldFrameReportsExternalSource)
{
    HrData m;
    m->mValue[HeartRate::BPM].f = 130.f;
    m->mValue[HeartRate::TRUST_LEVEL].f = 3.f;
    m->mValue[HeartRate::SOURCE].f = 2.f;  // EXTERNAL

    SDK::Sensor::DataView v(*m.data(), 3);
    HeartRate p(v);

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 130.f);
    EXPECT_EQ(src(p.getSource()), src(HeartRate::Source::EXTERNAL));
}

TEST(HeartRateParser, ThreeFieldFrameOpticalSource)
{
    HrData m;
    m->mValue[HeartRate::BPM].f = 68.f;
    m->mValue[HeartRate::TRUST_LEVEL].f = 2.f;
    m->mValue[HeartRate::SOURCE].f = 1.f;  // OPTICAL

    SDK::Sensor::DataView v(*m.data(), 3);
    HeartRate p(v);

    EXPECT_EQ(src(p.getSource()), src(HeartRate::Source::OPTICAL));
}

TEST(HeartRateParser, OneFieldFrameIsInvalid)
{
    HrData m;
    m->mValue[HeartRate::BPM].f = 72.f;

    SDK::Sensor::DataView v(*m.data(), 1);
    HeartRate p(v);

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
    EXPECT_EQ(src(p.getSource()), src(HeartRate::Source::UNKNOWN));
}
