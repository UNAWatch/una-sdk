/**
 * Host unit tests for SensorDataParser::Gyroscope — the cooked 3-axis angular
 * velocity frame (float X/Y/Z). Guards the getXYZ() bulk accessor against
 * reading the wrong union view: it must return the float values, not the raw
 * bit pattern reinterpreted through the int32 view.
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGyroscope.hpp"

using Gyroscope = SDK::SensorDataParser::Gyroscope;

namespace {

// A SDK::Sensor::Data with room for the 3 gyroscope fields.
struct GyroData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            3 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

void fill(GyroData& m, float x, float y, float z)
{
    m->mValue[Gyroscope::X].f = x;
    m->mValue[Gyroscope::Y].f = y;
    m->mValue[Gyroscope::Z].f = z;
}

} // namespace

// getXYZ() must return the float values that were written. This is the direct
// regression guard: reading the int32 view instead of the float view turns a
// stored 1.5f into its bit pattern (1069547520.f), so this fails on the bug.
TEST(GyroscopeParser, GetXYZReturnsFloatValues)
{
    GyroData m;
    fill(m, 1.5f, -2.25f, 0.125f);

    SDK::Sensor::DataView v(*m.data(), Gyroscope::COUNT);
    Gyroscope p(v);

    float x = 0.f, y = 0.f, z = 0.f;
    ASSERT_TRUE(p.getXYZ(x, y, z));

    EXPECT_FLOAT_EQ(x, 1.5f);
    EXPECT_FLOAT_EQ(y, -2.25f);
    EXPECT_FLOAT_EQ(z, 0.125f);
}

// getXYZ() and the scalar getX/getY/getZ() read the same frame, so they must
// agree. The scalar getters use the float view, so this also fails on the bug.
TEST(GyroscopeParser, GetXYZMatchesScalarGetters)
{
    GyroData m;
    fill(m, 12.5f, -0.5f, 3.75f);

    SDK::Sensor::DataView v(*m.data(), Gyroscope::COUNT);
    Gyroscope p(v);

    float x = 0.f, y = 0.f, z = 0.f;
    ASSERT_TRUE(p.getXYZ(x, y, z));

    EXPECT_FLOAT_EQ(x, p.getX());
    EXPECT_FLOAT_EQ(y, p.getY());
    EXPECT_FLOAT_EQ(z, p.getZ());
}

// A frame whose field count doesn't match the 3-axis layout is rejected, and
// getXYZ() reports failure.
TEST(GyroscopeParser, ShortFrameIsInvalid)
{
    GyroData m;
    fill(m, 1.f, 2.f, 3.f);

    SDK::Sensor::DataView v(*m.data(), Gyroscope::COUNT - 1);
    Gyroscope p(v);

    EXPECT_FALSE(p.isDataValid());

    float x = -1.f, y = -1.f, z = -1.f;
    EXPECT_FALSE(p.getXYZ(x, y, z));
}
