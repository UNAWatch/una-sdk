// Oval::TrackModel: geometry and projection.

#include "Oval/LaneTable.hpp"
#include "Oval/TrackModel.hpp"

#include <gtest/gtest.h>

#include <cmath>

using Oval::LaneTable;
using Oval::TrackModel;
using Oval::Vec2;

namespace {

const float kRadii[] = {35.0f, 35.5f, 36.0f, 36.5f, 37.0f, 37.5f, 38.0f};

TrackModel modelAt(float kerbR, bool clockwise = false, float headingDeg = 23.0f)
{
    TrackModel m;
    m.set({12.0f, -34.0f}, headingDeg * 3.14159265f / 180.0f, kerbR, clockwise);
    return m;
}

} // namespace

TEST(TrackModel, LapLengthMatchesTheLaneTableForEveryLaneAndRadius)
{
    const LaneTable t = LaneTable::outdoor();
    for (float r : kRadii) {
        const TrackModel m = modelAt(r);
        for (uint8_t lane = 1; lane <= 8; ++lane) {
            EXPECT_NEAR(m.lapLengthM(t.lineOffsetM(lane)), t.lapLengthM(lane), 0.01f)
                << "R " << r << " lane " << int(lane);
            // The same by walking the stations: s at the end of the lap.
            EXPECT_NEAR(m.sOf(3.99999f, t.lineOffsetM(lane)), t.lapLengthM(lane), 0.01f)
                << "R " << r << " lane " << int(lane);
        }
    }
}

TEST(TrackModel, StraightKeepsLane1At400m)
{
    for (float r : kRadii) {
        const TrackModel m = modelAt(r);
        const double expected = (400.0 - 2.0 * 3.141592653589793 * (r + 0.30)) / 2.0;
        EXPECT_NEAR(m.straightM(), expected, 1e-3) << "R " << r;
    }
    EXPECT_NEAR(modelAt(36.5f).straightM(), 84.39f, 0.01f);
}

TEST(TrackModel, PointsOnALineProjectBackWithZeroOffset)
{
    const LaneTable t = LaneTable::outdoor();
    for (bool cw : {false, true}) {
        for (float r : kRadii) {
            const TrackModel m = modelAt(r, cw);
            for (uint8_t lane = 1; lane <= 8; ++lane) {
                const float off = t.lineOffsetM(lane);
                for (int k = 0; k < 400; ++k) {
                    const float tau = 0.01f * k + 0.003f;
                    const Vec2  p   = m.pointAt(tau, off);
                    const TrackModel::Projection pr = m.project(p, off);
                    ASSERT_NEAR(pr.lateralM, 0.0f, 2e-3f) << "R " << r << " lane " << int(lane) << " tau " << tau;
                    ASSERT_NEAR(pr.tau, tau, 2e-4f) << "R " << r << " lane " << int(lane);
                    ASSERT_NEAR(pr.s, m.sOf(tau, off), 2e-3f);
                }
            }
        }
    }
}

TEST(TrackModel, LateralOffsetIsSignedOutward)
{
    const TrackModel m = modelAt(36.5f);
    for (float tau : {0.3f, 1.4f, 2.6f, 3.5f}) {
        for (float lat : {-6.0f, -1.0f, 2.5f, 12.0f}) {
            const TrackModel::Projection pr = m.project(m.pointAt(tau, 1.42f, lat), 1.42f);
            EXPECT_NEAR(pr.lateralM, lat, 2e-3f) << "tau " << tau;
            EXPECT_NEAR(pr.tau, tau, 1e-4f);
        }
        // The normal points the same way.
        const Vec2 n  = m.normalAt(tau);
        const Vec2 a  = m.pointAt(tau, 0.3f);
        const Vec2 b  = m.pointAt(tau, 0.3f, 1.0f);
        EXPECT_NEAR(b.x - a.x, n.x, 1e-3f);
        EXPECT_NEAR(b.y - a.y, n.y, 1e-3f);
    }
}

TEST(TrackModel, StationsAreTheSamePlaceInEveryLane)
{
    const LaneTable  t = LaneTable::outdoor();
    const TrackModel m = modelAt(37.2f);
    for (float tau : {0.0f, 0.5f, 1.25f, 2.0f, 3.75f}) {
        const Vec2 p1 = m.pointAt(tau, t.lineOffsetM(1));
        const Vec2 p8 = m.pointAt(tau, t.lineOffsetM(8));
        // Lane 8's point is straight out from lane 1's, along the normal.
        const Vec2 n = m.normalAt(tau);
        const Vec2 d = p8 - p1;
        EXPECT_NEAR(Oval::dot(d, n), t.lineOffsetM(8) - t.lineOffsetM(1), 2e-3f);
        EXPECT_NEAR(d.x * n.y - d.y * n.x, 0.0f, 2e-3f);
        // tauOf and sOf invert each other.
        for (int lane : {1, 4, 8}) {
            EXPECT_NEAR(m.tauOf(m.sOf(tau, t.lineOffsetM(uint8_t(lane))), t.lineOffsetM(uint8_t(lane))), tau, 1e-4f);
        }
    }
}

TEST(TrackModel, RunningDirectionAndHeadingNormalisation)
{
    // Anticlockwise: s grows anticlockwise round the centre.
    for (bool cw : {false, true}) {
        const TrackModel m  = modelAt(36.5f, cw);
        const Vec2       a  = m.pointAt(0.2f, 0.3f) - m.centre();
        const Vec2       b  = m.pointAt(0.3f, 0.3f) - m.centre();
        const float      cr = a.x * b.y - a.y * b.x;
        EXPECT_EQ(cr > 0.0f, !cw);
    }
    // A heading half a turn round is the same track and the same stations.
    const TrackModel m1 = modelAt(36.5f, false, 200.0f);
    const TrackModel m2 = modelAt(36.5f, false, 20.0f);
    EXPECT_NEAR(m1.headingDeg(), 20.0f, 1e-3f);
    for (float tau : {0.1f, 1.7f, 3.2f}) {
        const Vec2 p = m2.pointAt(tau, 2.64f);
        EXPECT_NEAR(m1.project(p, 2.64f).tau, tau, 1e-4f);
    }
}

TEST(TrackModel, GeoRoundTrip)
{
    Oval::LocalFrame f1, f2;
    f1.setOrigin(55.95, -3.19);
    f2.setOrigin(55.951, -3.188);
    TrackModel m = modelAt(36.5f);
    m.setGeo(f1);
    ASSERT_TRUE(m.hasGeo());
    const Vec2 world = m.pointAt(1.3f, 0.3f); // in f1
    double lat = 0.0, lon = 0.0;
    f1.toLatLon(world, lat, lon);
    ASSERT_TRUE(m.placeIn(f2));
    const TrackModel::Projection pr = m.project(f2.toLocal(lat, lon), 0.3f);
    EXPECT_NEAR(pr.lateralM, 0.0f, 0.01f);
    EXPECT_NEAR(pr.tau, 1.3f, 1e-4f);

    const TrackModel g = TrackModel::fromGeo(m.centreLat(), m.centreLon(), m.headingDeg(), 36.5f, false, 51.0f);
    TrackModel placed = g;
    ASSERT_TRUE(placed.placeIn(f2));
    EXPECT_NEAR(placed.project(f2.toLocal(lat, lon), 0.3f).tau, 1.3f, 1e-4f);
    EXPECT_FLOAT_EQ(placed.altitudeM(), 51.0f);
}
