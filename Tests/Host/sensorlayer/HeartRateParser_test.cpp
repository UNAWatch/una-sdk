/**
 * Host unit tests for SensorDataParser::HeartRate — the stable 2-field
 * (BPM, TRUST_LEVEL) layout. HR provenance and per-source readings live in the
 * opt-in HEART_RATE_EX type (see HeartRateExParser_test.cpp).
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"

using HeartRate = SDK::SensorDataParser::HeartRate;

namespace {

// A SDK::Sensor::Data with room for a few float fields.
struct HrData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            3 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

} // namespace

TEST(HeartRateParser, TwoFieldFrameValid)
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
    EXPECT_EQ(HeartRate::getFieldsNumber(), 2);
}

TEST(HeartRateParser, OneFieldFrameIsInvalid)
{
    HrData m;
    m->mValue[HeartRate::BPM].f = 72.f;

    SDK::Sensor::DataView v(*m.data(), 1);
    HeartRate p(v);

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
    EXPECT_FLOAT_EQ(p.getTrustLevel(), 0.f);
}

TEST(HeartRateParser, ThreeFieldFrameIsInvalid)
{
    // The 2-field layout validates on an exact field count, so an over-long
    // frame is rejected rather than silently misread.
    HrData m;
    m->mValue[HeartRate::BPM].f = 130.f;
    m->mValue[HeartRate::TRUST_LEVEL].f = 3.f;
    m->mValue[2].f = 2.f;

    SDK::Sensor::DataView v(*m.data(), 3);
    HeartRate p(v);

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
}
