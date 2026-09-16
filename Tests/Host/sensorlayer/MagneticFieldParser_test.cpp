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

// The refusal past the calibration check: gravity that cannot say which way
// is down must not write a bearing either.
TEST(MagneticFieldParser, ATiltedRefusalLeavesTheBearingAlone)
{
    MagData m;
    fill(m, 0.0f, kStrong, 0.0f, true);

    MagneticField p = parse(m);

    float degrees = -1.0f;
    EXPECT_FALSE(p.getAzimuthDegTilted(0.0f, 0.0f, 0.0f, degrees));
    EXPECT_FLOAT_EQ(degrees, -1.0f) << "wrote a bearing it had refused to give";
}

// The sign of gravity is a contract nothing can check at run time, because a
// watch lying face-down is a real attitude. The accelerometer reads az
// positive face-up; a vector pointing down, passed instead, mirrors the
// bearing rather than turning it round, so it does not look wrong.
TEST(MagneticFieldParser, GravityIsTheAccelerometerReadingNotTheDownVector)
{
    MagData m;
    fill(m, 10.0f, 20.0f, -40.0f, true);

    MagneticField p = parse(m);

    float faceUp = 0.0f;
    ASSERT_TRUE(p.getAzimuthDegTilted(0.0f, 0.0f, 1.0f, faceUp));
    EXPECT_NEAR(faceUp, 333.435f, 0.01f);
    EXPECT_NEAR(faceUp, p.getAzimuthDeg(), 0.01f);

    float downVector = 0.0f;
    ASSERT_TRUE(p.getAzimuthDegTilted(0.0f, 0.0f, -1.0f, downVector));
    EXPECT_NEAR(downVector, 360.0f - faceUp, 0.01f)
            << "the down vector did not mirror the bearing";
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

// A model of the watch in the world, for the tests below: the answer to every
// question they ask is then the heading the watch was pointed at, known without
// any of the arithmetic under test.
//
// The world is east, north, up. The field is a northern one: 49 uT at 66.5
// degrees of inclination, whose vertical part is more than twice the
// horizontal - the case where tilt turns a level bearing round.
namespace {

struct Vec3 {
    float x;
    float y;
    float z;
};

float dot(Vec3 a, Vec3 b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

constexpr float kDegToRad = 3.14159265f / 180.0f;

/// v turned by `deg` about the unit axis k, right-handed.
Vec3 turn(Vec3 v, Vec3 k, float deg)
{
    const float c = std::cos(deg * kDegToRad);
    const float s = std::sin(deg * kDegToRad);
    const float d = dot(v, k);
    const Vec3  kxv{(k.y * v.z) - (k.z * v.y), (k.z * v.x) - (k.x * v.z),
                    (k.x * v.y) - (k.y * v.x)};

    return {(v.x * c) + (kxv.x * s) + (k.x * d * (1.0f - c)),
            (v.y * c) + (kxv.y * s) + (k.y * d * (1.0f - c)),
            (v.z * c) + (kxv.z * s) + (k.z * d * (1.0f - c))};
}

struct Reading {
    Vec3 field;     ///< What the magnetometer reads, watch axes.
    Vec3 gravity;   ///< What the accelerometer reads, watch axes: up.
};

/// The watch with 12 o'clock pointed at `headingDeg`, then raised by
/// `raiseDeg` (the arm lifted: a turn about 3-9 o'clock), then the wrist turned
/// by `rollDeg` about the forearm (12-6 o'clock), positive lowering 3 o'clock.
Reading watchAt(float headingDeg, float raiseDeg, float rollDeg)
{
    const float h = headingDeg * kDegToRad;

    Vec3 x{std::cos(h), -std::sin(h), 0.0f};
    Vec3 y{std::sin(h), std::cos(h), 0.0f};
    Vec3 z{0.0f, 0.0f, 1.0f};

    y = turn(y, x, raiseDeg);
    z = turn(z, x, raiseDeg);

    x = turn(x, y, rollDeg);
    z = turn(z, y, rollDeg);

    const float inc = 66.5f * kDegToRad;
    const Vec3  field{0.0f, 49.0f * std::cos(inc), -49.0f * std::sin(inc)};
    const Vec3  up{0.0f, 0.0f, 1.0f};

    return {{dot(field, x), dot(field, y), dot(field, z)},
            {dot(up, x), dot(up, y), dot(up, z)}};
}

bool bearingAt(const Reading& r, float& degrees)
{
    float xh = 0.0f;
    float yh = 0.0f;

    if (!MagneticField::levelProject(r.field.x, r.field.y, r.field.z,
                                     r.gravity.x, r.gravity.y, r.gravity.z,
                                     xh, yh)) {
        return false;
    }

    if (!MagneticField::hasDirection(xh, yh)) {
        return false;
    }

    degrees = MagneticField::bearingDeg(xh, yh);
    return true;
}

float angleApart(float a, float b)
{
    const float d = std::fmod(std::fabs(a - b), 360.0f);
    return (d > 180.0f) ? (360.0f - d) : d;
}

} // namespace

// Where a tilted bearing is taken for granted: the arm raised or lowered, the
// wrist turned about the forearm, both at once, in every direction. The wrist
// goes all the way round, so the face on edge and face down are in here too;
// neither changes where 12 o'clock points.
//
// Splitting a tilt into two angles and undoing them one at a time passes every
// case below that has only one of the two - and misses the others by tens of
// degrees. That is why the grid has both.
TEST(MagneticFieldParser, ArmRaisedAndWristTurnedKeepTheBearing)
{
    int checked = 0;

    for (int heading = 0; heading < 360; heading += 45) {
        for (int raise = -75; raise <= 75; raise += 15) {
            for (int roll = -180; roll <= 180; roll += 30) {
                const Reading r = watchAt(static_cast<float>(heading),
                                          static_cast<float>(raise),
                                          static_cast<float>(roll));

                float degrees = -1.0f;
                ASSERT_TRUE(bearingAt(r, degrees))
                        << "heading " << heading << " raise " << raise
                        << " roll " << roll;

                EXPECT_LT(angleApart(degrees, static_cast<float>(heading)), 0.1f)
                        << "heading " << heading << " raise " << raise
                        << " roll " << roll;

                checked++;
            }
        }
    }

    EXPECT_EQ(checked, 8 * 11 * 13) << "the grid stopped early";
}

// The same through the parser, the way an app calls it.
TEST(MagneticFieldParser, TheTiltedBearingIsTheBearingOfTwelveOClock)
{
    const Reading r = watchAt(120.0f, 30.0f, 45.0f);

    MagData m;
    fill(m, r.field.x, r.field.y, r.field.z, true);

    MagneticField p = parse(m);

    float degrees = -1.0f;
    ASSERT_TRUE(p.getAzimuthDegTilted(r.gravity.x, r.gravity.y, r.gravity.z,
                                      degrees));
    EXPECT_LT(angleApart(degrees, 120.0f), 0.1f);

    // Gravity in any unit: an accelerometer in m/s^2 reads the same bearing.
    float inMs2 = -1.0f;
    ASSERT_TRUE(p.getAzimuthDegTilted(r.gravity.x * 9.80665f,
                                      r.gravity.y * 9.80665f,
                                      r.gravity.z * 9.80665f, inMs2));
    EXPECT_NEAR(inMs2, degrees, 0.01f);
}

// The invariant the whole projection rests on: gravity, fed in as if it were
// the field, must project to nothing. It points straight up by definition, so
// a transform that puts the horizontal plane where the horizontal plane really
// is has to leave it no horizontal part at all.
TEST(MagneticFieldParser, ProjectingGravityItselfLeavesNothingHorizontal)
{
    int checked = 0;

    for (int raise = -75; raise <= 75; raise += 15) {
        for (int roll = -180; roll <= 180; roll += 30) {
            const Reading r = watchAt(0.0f, static_cast<float>(raise),
                                      static_cast<float>(roll));
            const Vec3& g = r.gravity;

            float xh = 0.0f;
            float yh = 0.0f;

            ASSERT_TRUE(MagneticField::levelProject(g.x, g.y, g.z,
                                                    g.x, g.y, g.z, xh, yh))
                    << "at raise " << raise << " roll " << roll;

            EXPECT_NEAR(xh, 0.0f, 1e-4f)
                    << "gravity leaked into X at raise " << raise
                    << " roll " << roll;
            EXPECT_NEAR(yh, 0.0f, 1e-4f)
                    << "gravity leaked into Y at raise " << raise
                    << " roll " << roll;

            checked++;
        }
    }

    EXPECT_EQ(checked, 11 * 13) << "the grid stopped early";
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

// With 12 o'clock pointing at the sky or the ground the forearm has no
// direction on the map, and a bearing computed anyway would spin freely.
TEST(MagneticFieldParser, TwelveOClockUprightHasNoBearing)
{
    float degrees = 123.0f;

    EXPECT_FALSE(bearingAt(watchAt(0.0f, 90.0f, 0.0f), degrees)) << "pointing up";
    EXPECT_FALSE(bearingAt(watchAt(0.0f, -90.0f, 0.0f), degrees)) << "pointing down";
    EXPECT_FLOAT_EQ(degrees, 123.0f) << "wrote a bearing it had refused to give";
}

// The watch edge-on with 3 o'clock at the ground still has 12 o'clock level,
// so it has a bearing; so does a watch lying face down.
TEST(MagneticFieldParser, EdgeOnAndFaceDownStillHaveABearing)
{
    float degrees = -1.0f;

    ASSERT_TRUE(bearingAt(watchAt(90.0f, 0.0f, 90.0f), degrees)) << "3 o'clock down";
    EXPECT_LT(angleApart(degrees, 90.0f), 0.1f);

    ASSERT_TRUE(bearingAt(watchAt(90.0f, 0.0f, -90.0f), degrees)) << "3 o'clock up";
    EXPECT_LT(angleApart(degrees, 90.0f), 0.1f);

    ASSERT_TRUE(bearingAt(watchAt(90.0f, 0.0f, 180.0f), degrees)) << "face down";
    EXPECT_LT(angleApart(degrees, 90.0f), 0.1f);
}

// Where the upright gate falls, not only that it exists: either side of a
// share of 12 o'clock left flat of 0.03 squared, about 10 degrees from upright,
// whichever way the wrist is turned. Gravity has unit length, so that square
// is exactly what is set here.
TEST(MagneticFieldParser, TheUprightGateFallsWhereItSays)
{
    auto project = [](float flatSq, float rollDeg) {
        const float c = std::sqrt(flatSq);
        const float r = rollDeg * kDegToRad;
        float xh = 0.0f;
        float yh = 0.0f;
        return MagneticField::levelProject(0.0f, 0.0f, kStrong,
                                           -c * std::sin(r),
                                           std::sqrt(1.0f - flatSq),
                                           c * std::cos(r), xh, yh);
    };

    for (int roll = -180; roll <= 180; roll += 45) {
        EXPECT_TRUE(project(0.04f, static_cast<float>(roll)))
                << "refused just inside the gate, roll " << roll;
        EXPECT_FALSE(project(0.02f, static_cast<float>(roll)))
                << "accepted just outside the gate, roll " << roll;
    }
}
