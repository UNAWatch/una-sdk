// Oval::LocalFrame against great-circle (haversine) distances.

#include "Oval/LocalFrame.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

using Oval::LocalFrame;
using Oval::Vec2;

namespace {

constexpr double kR   = LocalFrame::kEarthRadiusM;
constexpr double kRad = 3.141592653589793 / 180.0;

double haversineM(double lat1, double lon1, double lat2, double lon2)
{
    const double dLat = (lat2 - lat1) * kRad;
    const double dLon = (lon2 - lon1) * kRad;
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2)
        + std::cos(lat1 * kRad) * std::cos(lat2 * kRad) * std::sin(dLon / 2) * std::sin(dLon / 2);
    return 2.0 * kR * std::asin(std::sqrt(a));
}

// The point distM away on a great circle at the given bearing.
void destination(double lat, double lon, double bearingDeg, double distM, double& lat2, double& lon2)
{
    const double d  = distM / kR;
    const double b  = bearingDeg * kRad;
    const double p1 = lat * kRad;
    const double p2 = std::asin(std::sin(p1) * std::cos(d) + std::cos(p1) * std::sin(d) * std::cos(b));
    const double l2 = lon * kRad + std::atan2(std::sin(b) * std::sin(d) * std::cos(p1),
                                              std::cos(d) - std::sin(p1) * std::sin(p2));
    lat2 = p2 / kRad;
    lon2 = l2 / kRad;
}

double worstRelativeError(double lat0, double lon0)
{
    LocalFrame f;
    f.setOrigin(lat0, lon0);
    double worst = 0.0;
    const double dists[] = {1.0, 5.0, 10.0, 50.0, 100.0, 250.0, 400.0, 500.0, 750.0, 1000.0};
    for (double bearing = 0.0; bearing < 360.0; bearing += 7.5) {
        for (double dist : dists) {
            double lat = 0.0, lon = 0.0;
            destination(lat0, lon0, bearing, dist, lat, lon);
            const double ref = haversineM(lat0, lon0, lat, lon);
            const double got = Oval::length(f.toLocal(lat, lon));
            const double err = std::fabs(got - ref) / ref;
            if (!std::isfinite(err)) {
                ADD_FAILURE() << "bearing " << bearing << ", " << dist << " m: " << got << " m";
                return std::numeric_limits<double>::infinity();
            }
            worst = std::max(worst, err);
        }
    }
    return worst;
}

} // namespace

TEST(LocalFrame, MatchesHaversineAtEdinburgh)
{
    const double worst = worstRelativeError(55.95, -3.19);
    EXPECT_LT(worst, 0.001);
    std::printf("[ info     ] Edinburgh, 0 to 1 km: worst error against haversine %.5f%%\n", 100.0 * worst);
}

TEST(LocalFrame, MatchesHaversineAtTheEquator)
{
    const double worst = worstRelativeError(0.0, 30.0);
    EXPECT_LT(worst, 0.001);
    std::printf("[ info     ] equator, 0 to 1 km: worst error against haversine %.5f%%\n", 100.0 * worst);
}

TEST(LocalFrame, AxesAreEastAndNorth)
{
    LocalFrame f;
    f.setOrigin(55.95, -3.19);
    double lat = 0.0, lon = 0.0;
    destination(55.95, -3.19, 0.0, 100.0, lat, lon);
    Vec2 p = f.toLocal(lat, lon);
    EXPECT_NEAR(p.x, 0.0f, 1e-3f);
    EXPECT_NEAR(p.y, 100.0f, 0.05f);
    destination(55.95, -3.19, 90.0, 100.0, lat, lon);
    p = f.toLocal(lat, lon);
    EXPECT_NEAR(p.x, 100.0f, 0.05f);
    EXPECT_NEAR(p.y, 0.0f, 0.01f); // a great circle east drifts south a hair
}

TEST(LocalFrame, RoundTrips)
{
    LocalFrame f;
    f.setOrigin(-33.9, 151.2);
    for (float x = -300.0f; x <= 300.0f; x += 37.5f) {
        for (float y = -300.0f; y <= 300.0f; y += 41.0f) {
            double lat = 0.0, lon = 0.0;
            f.toLatLon({x, y}, lat, lon);
            const Vec2 back = f.toLocal(lat, lon);
            EXPECT_NEAR(back.x, x, 1e-3f);
            EXPECT_NEAR(back.y, y, 1e-3f);
        }
    }
}

TEST(LocalFrame, HandlesTheAntimeridian)
{
    LocalFrame f;
    f.setOrigin(10.0, 179.9995);
    const Vec2 p = f.toLocal(10.0, -179.9995);
    EXPECT_NEAR(p.x, 0.001 * kR * kRad * std::cos(10.0 * kRad), 0.01);
    EXPECT_NEAR(p.y, 0.0f, 1e-3f);
}

// The sensor gives float degrees: about 0.4 m north-south resolution at
// Edinburgh. Converting at once to local metres keeps that, no worse.
TEST(LocalFrame, FloatCoordinatesStayWithinHalfAMetre)
{
    LocalFrame f;
    f.setOrigin(55.95, -3.19);
    for (double d = 0.0; d <= 200.0; d += 13.0) {
        double lat = 0.0, lon = 0.0;
        destination(55.95, -3.19, 37.0, d, lat, lon);
        const Vec2 exact = f.toLocal(lat, lon);
        const Vec2 rounded = f.toLocal(static_cast<float>(lat), static_cast<float>(lon));
        EXPECT_LT(Oval::length(exact - rounded), 0.5f);
    }
}
