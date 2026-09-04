/**
 * Host unit tests for SensorDataParser::MagneticFieldRaw - the field as the
 * part measured it.
 *
 * Deliberately a thin frame: three floats and nothing derived from them. The
 * guard that matters is that it stays that way, and stays distinguishable from
 * the corrected frame, which carries one more field and validates on an exact
 * count.
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserMagneticField.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserMagneticFieldRaw.hpp"

using MagneticField    = SDK::SensorDataParser::MagneticField;
using MagneticFieldRaw = SDK::SensorDataParser::MagneticFieldRaw;

namespace {

// A SDK::Sensor::Data with room for the raw field's fields.
struct RawData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            MagneticFieldRaw::COUNT * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

} // namespace

TEST(MagneticFieldRawParser, ReadsBackWhatWasWritten)
{
    RawData m;
    m->mValue[MagneticFieldRaw::MAG_X].f =  1.5f;
    m->mValue[MagneticFieldRaw::MAG_Y].f = -2.25f;
    m->mValue[MagneticFieldRaw::MAG_Z].f =  0.125f;

    MagneticFieldRaw p(
            SDK::Sensor::DataView(*m.data(), MagneticFieldRaw::COUNT));

    ASSERT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getX(),  1.5f);
    EXPECT_FLOAT_EQ(p.getY(), -2.25f);
    EXPECT_FLOAT_EQ(p.getZ(),  0.125f);
}

// The two frames are different shapes, and every parser validates on an exact
// field count. A raw frame read as a corrected one must therefore refuse
// rather than report three good axes and an uninitialised fourth field as a
// calibration that was never applied.
TEST(MagneticFieldRawParser, ARawFrameIsNotACorrectedOne)
{
    EXPECT_NE(MagneticFieldRaw::getFieldsNumber(),
              MagneticField::getFieldsNumber());

    RawData m;
    MagneticField wrong(
            SDK::Sensor::DataView(*m.data(), MagneticFieldRaw::COUNT));

    EXPECT_FALSE(wrong.isDataValid());
    EXPECT_FALSE(wrong.isCalibrated());
    EXPECT_FALSE(wrong.isAzimuthValid());
}
