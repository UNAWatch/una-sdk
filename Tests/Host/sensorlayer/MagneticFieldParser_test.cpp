/**
 * Host unit tests for SensorDataParser::MagneticField - the corrected 3-axis
 * field, and the compass arithmetic that reads it.
 *
 * The bearing is a function of one sample, and of gravity when the caller
 * supplies it, with no state and no hardware behind it. That is what lets it
 * be checked here against fields whose answer is known by inspection, and it
 * is why the trigonometry lives in the parser rather than in a sensor.
 */

#include <cmath>
#include <cstdint>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserMagneticField.hpp"

using MagneticField = SDK::SensorDataParser::MagneticField;

namespace {

/// Field strengths well above the "is there a direction at all" floor.
constexpr float kStrong = 30.0f;

// A SDK::Sensor::Data with room for the magnetic field's fields.
struct MagData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            MagneticField::COUNT * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

void fill(MagData& m, float x, float y, float z, bool calibrated)
{
    m->mValue[MagneticField::MAG_X].f = x;
    m->mValue[MagneticField::MAG_Y].f = y;
    m->mValue[MagneticField::MAG_Z].f = z;
    m->mValue[MagneticField::MAG_CALIBRATED].u32 = calibrated ? 1U : 0U;
}

MagneticField parse(MagData& m)
{
    return MagneticField(SDK::Sensor::DataView(*m.data(), MagneticField::COUNT));
}

} // namespace

// -----------------------------------------------------------------------------
// The frame
// -----------------------------------------------------------------------------

TEST(MagneticFieldParser, ReadsBackWhatWasWritten)
{
    MagData m;
    fill(m, 1.5f, -2.25f, 0.125f, true);

    MagneticField p = parse(m);

    ASSERT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getX(),  1.5f);
    EXPECT_FLOAT_EQ(p.getY(), -2.25f);
    EXPECT_FLOAT_EQ(p.getZ(),  0.125f);
    EXPECT_TRUE(p.isCalibrated());
}

// An uncalibrated sample still carries a field - that is what a calibration run
// consumes - but nothing may take a bearing from it.
TEST(MagneticFieldParser, AnUncalibratedSampleHasNoBearing)
{
    MagData m;
    fill(m, 0.0f, kStrong, 0.0f, false);

    MagneticField p = parse(m);

    EXPECT_FALSE(p.isCalibrated());
    EXPECT_FALSE(p.isAzimuthValid());
    EXPECT_FLOAT_EQ(p.getAzimuthDeg(), 0.0f);

    float degrees = -1.0f;
    EXPECT_FALSE(p.getAzimuthDegTilted(0.0f, 0.0f, 1.0f, degrees));
    EXPECT_FLOAT_EQ(degrees, -1.0f) << "wrote a bearing it had refused to give";
}

TEST(MagneticFieldParser, ACalibratedSampleGivesABearing)
{
    MagData m;
    fill(m, 0.0f, kStrong, 0.0f, true);

    MagneticField p = parse(m);

    ASSERT_TRUE(p.isAzimuthValid());
    EXPECT_NEAR(p.getAzimuthDeg(), 0.0f, 0.01f);
}

// Held flat, the two bearings are the same arithmetic and must agree.
TEST(MagneticFieldParser, TiltedAndLevelAgreeWhenTheWatchIsLevel)
{
    MagData m;
    fill(m, -12.0f, 34.0f, -56.0f, true);

    MagneticField p = parse(m);

    float tilted = 0.0f;
    ASSERT_TRUE(p.getAzimuthDegTilted(0.0f, 0.0f, 1.0f, tilted));

    EXPECT_NEAR(tilted, p.getAzimuthDeg(), 0.01f);
}

// -----------------------------------------------------------------------------
// The arithmetic, exercised directly
// -----------------------------------------------------------------------------

/// The bearings a level watch reads for a field pointing along each axis.
/// +Y is 12 o'clock, and a bearing is of the 12 o'clock direction, so a field
/// pointing out of 12 o'clock means the watch is facing north.
TEST(MagneticFieldParser, LevelCardinalDirections)
{
    EXPECT_NEAR(MagneticField::bearingDeg(0.0f,  kStrong),   0.0f, 0.01f) << "north";
    EXPECT_NEAR(MagneticField::bearingDeg(-kStrong, 0.0f),  90.0f, 0.01f) << "east";
    EXPECT_NEAR(MagneticField::bearingDeg(0.0f, -kStrong), 180.0f, 0.01f) << "south";
    EXPECT_NEAR(MagneticField::bearingDeg(kStrong,  0.0f), 270.0f, 0.01f) << "west";
}

TEST(MagneticFieldParser, BearingStaysInRange)
{
    // Round the circle in degrees, which is the range a caller is promised and
    // the one a screen will index a compass rose with.
    for (int i = -720; i <= 720; i++) {
        const float radians = static_cast<float>(i) * 3.14159265f / 180.0f;
        const float degrees =
                MagneticField::bearingDeg(std::sin(radians) * kStrong,
                                          std::cos(radians) * kStrong);

        EXPECT_GE(degrees, 0.0f);
        EXPECT_LT(degrees, 360.0f);
    }
}

TEST(MagneticFieldParser, AFieldOfNothingHasNoDirection)
{
    EXPECT_FALSE(MagneticField::hasDirection(0.0f, 0.0f));
    EXPECT_FALSE(MagneticField::hasDirection(1.0f, 1.0f)) << "below the floor";
    EXPECT_TRUE(MagneticField::hasDirection(kStrong, 0.0f));
}

TEST(MagneticFieldParser, ANaNFieldHasNoDirection)
{
    const float nan = std::nanf("");

    EXPECT_FALSE(MagneticField::hasDirection(nan, kStrong));
    EXPECT_FALSE(MagneticField::hasDirection(kStrong, nan));
}

// -----------------------------------------------------------------------------
// Tilt
// -----------------------------------------------------------------------------

TEST(MagneticFieldParser, LevelProjectionIsTheIdentityWhenTheWatchIsLevel)
{
    float xh = 0.0f;
    float yh = 0.0f;

    ASSERT_TRUE(MagneticField::levelProject(12.0f, -34.0f, 56.0f,
                                            0.0f, 0.0f, 1.0f, xh, yh));

    EXPECT_NEAR(xh,  12.0f, 0.001f);
    EXPECT_NEAR(yh, -34.0f, 0.001f);
}

// The point of the whole exercise: tip the watch and the bearing must not move.
// The field is fixed in the world; only the watch turns, so the measured field
// is the world field expressed in the watch's axes.
TEST(MagneticFieldParser, TiltingTheWatchDoesNotMoveTheBearing)
{
    // A field pointing north and downwards, as it does in the northern
    // hemisphere: 20 uT along +Y, 40 uT into the watch face.
    const float northUt = 20.0f;
    const float downUt  = 40.0f;

    float level = 0.0f;
    {
        float xh = 0.0f;
        float yh = 0.0f;
        ASSERT_TRUE(MagneticField::levelProject(0.0f, northUt, -downUt,
                                                0.0f, 0.0f, 1.0f, xh, yh));
        level = MagneticField::bearingDeg(xh, yh);
    }

    // Pitch the watch nose-down by an angle: both the field and gravity rotate
    // in the watch's axes by the same rotation about X.
    for (int degrees = -40; degrees <= 40; degrees += 10) {
        const float a = static_cast<float>(degrees) * 3.14159265f / 180.0f;
        const float c = std::cos(a);
        const float s = std::sin(a);

        const float my = (northUt * c) - (-downUt * s);
        const float mz = (northUt * s) + (-downUt * c);

        const float ay = -s;
        const float az =  c;

        float xh = 0.0f;
        float yh = 0.0f;
        ASSERT_TRUE(MagneticField::levelProject(0.0f, my, mz,
                                                0.0f, ay, az, xh, yh))
                << "at " << degrees << " deg";

        EXPECT_NEAR(MagneticField::bearingDeg(xh, yh), level, 0.1f)
                << "bearing moved when only the watch did, at "
                << degrees << " deg";
    }
}

// The invariant the whole projection rests on, and the one that does not care
// which Euler convention anyone had in mind: gravity, fed in as if it were the
// field, must project to nothing. It points straight down by definition, so a
// transform that puts the horizontal plane where the horizontal plane really
// is has to leave it no horizontal part at all.
//
// This is what separates a projection built from the angles it extracted from
// one built from a different composition order. Those two agree exactly while
// only one of pitch and roll is non-zero, so a test that tilts about a single
// axis cannot tell them apart - and every direction below has both.
TEST(MagneticFieldParser, ProjectingGravityItselfLeavesNothingHorizontal)
{
    int checked = 0;

    for (int pitchDeg = -70; pitchDeg <= 70; pitchDeg += 10) {
        for (int rollDeg = -70; rollDeg <= 70; rollDeg += 10) {
            const float p = static_cast<float>(pitchDeg) * 3.14159265f / 180.0f;
            const float r = static_cast<float>(rollDeg) * 3.14159265f / 180.0f;

            // Gravity as the watch would read it at this attitude.
            const float gx = -std::sin(p);
            const float gy =  std::sin(r) * std::cos(p);
            const float gz =  std::cos(r) * std::cos(p);

            float xh = 0.0f;
            float yh = 0.0f;

            ASSERT_TRUE(MagneticField::levelProject(gx, gy, gz,
                                                    gx, gy, gz, xh, yh))
                    << "at pitch " << pitchDeg << " roll " << rollDeg;

            EXPECT_NEAR(xh, 0.0f, 1e-4f)
                    << "gravity leaked into X at pitch " << pitchDeg
                    << " roll " << rollDeg;
            EXPECT_NEAR(yh, 0.0f, 1e-4f)
                    << "gravity leaked into Y at pitch " << pitchDeg
                    << " roll " << rollDeg;

            checked++;
        }
    }

    EXPECT_GT(checked, 100) << "the grid stopped early";
}

// The other half of the same guarantee, and physical rather than algebraic:
// turning the watch about the vertical is what a bearing is supposed to
// measure, so at any fixed tilt the reported bearing must follow that turn
// one for one.
//
// The tilt here has both pitch and roll, which is the case the single-axis
// test above cannot reach. A projection built from a different composition
// order than the angles it extracted still returns a smoothly varying number
// here - it simply returns the wrong one, off by an amount that depends on the
// tilt, so it neither tracks the turn nor agrees with the level reading.
TEST(MagneticFieldParser, AtAnyTiltTheBearingFollowsATurnAboutTheVertical)
{
    // North along +Y and downwards, as in the northern hemisphere.
    const float worldField[3] = { 0.0f, 20.0f, -40.0f };
    const float worldUp[3]    = { 0.0f, 0.0f, 1.0f };

    // A tilt held fixed relative to the watch while it is turned underneath.
    const float pitch = 30.0f * 3.14159265f / 180.0f;
    const float roll  = 25.0f * 3.14159265f / 180.0f;

    auto toBody = [&](const float v[3], float yaw, float out[3]) {
        // Turn about the world vertical first, then tilt in the turned frame,
        // so the turn is a heading change and the tilt is not.
        const float cy = std::cos(-yaw);
        const float sy = std::sin(-yaw);
        float a[3] = { (v[0] * cy) - (v[1] * sy),
                       (v[0] * sy) + (v[1] * cy),
                       v[2] };

        const float cp = std::cos(pitch);
        const float sp = std::sin(pitch);
        float b[3] = { (a[0] * cp) + (a[2] * sp),
                       a[1],
                       -(a[0] * sp) + (a[2] * cp) };

        const float cr = std::cos(roll);
        const float sr = std::sin(roll);
        out[0] = b[0];
        out[1] = (b[1] * cr) - (b[2] * sr);
        out[2] = (b[1] * sr) + (b[2] * cr);
    };

    float reference = 0.0f;

    for (int deg = 0; deg <= 180; deg += 15) {
        const float yaw = static_cast<float>(deg) * 3.14159265f / 180.0f;

        float m[3];
        float g[3];
        toBody(worldField, yaw, m);
        toBody(worldUp, yaw, g);

        float xh = 0.0f;
        float yh = 0.0f;
        ASSERT_TRUE(MagneticField::levelProject(m[0], m[1], m[2],
                                                g[0], g[1], g[2], xh, yh))
                << "at yaw " << deg;

        const float got = MagneticField::bearingDeg(xh, yh);

        if (deg == 0) {
            reference = got;

            // Tilt alone must not move it: with the watch level and unturned
            // the field points along +Y, which is north.
            EXPECT_NEAR(std::fmod(got + 360.0f, 360.0f), 0.0f, 0.2f)
                    << "a tilt alone moved the bearing off north";
            continue;
        }

        // Turning the watch one way moves the bearing the other.
        const float expected = std::fmod((reference - static_cast<float>(deg))
                                         + 720.0f, 360.0f);

        EXPECT_NEAR(std::fmod(got + 360.0f, 360.0f), expected, 0.2f)
                << "the bearing did not follow the turn at yaw " << deg;
    }
}

TEST(MagneticFieldParser, GravityThatSaysNothingIsRefused)
{
    float xh = 0.0f;
    float yh = 0.0f;

    EXPECT_FALSE(MagneticField::levelProject(kStrong, 0.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, xh, yh))
            << "accepted a gravity vector of zero length";

    const float nan = std::nanf("");
    EXPECT_FALSE(MagneticField::levelProject(kStrong, 0.0f, 0.0f,
                                             nan, 0.0f, 1.0f, xh, yh))
            << "accepted a NaN gravity vector";
}

// Held edge-on, with the 3 o'clock side pointing at the ground, gravity cannot
// say how far the watch is rolled about that axis, and a bearing computed
// anyway would spin freely.
TEST(MagneticFieldParser, EdgeOnHasNoBearing)
{
    float xh = 0.0f;
    float yh = 0.0f;

    EXPECT_FALSE(MagneticField::levelProject(0.0f, kStrong, 0.0f,
                                             -1.0f, 0.0f, 0.0f, xh, yh));
}
