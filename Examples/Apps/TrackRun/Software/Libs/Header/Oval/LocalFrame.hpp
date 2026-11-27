/*******************************************************************************
 * @file   LocalFrame.hpp
 * @brief  Oval: latitude and longitude to local x (east) and y (north) metres
 *         around an origin, by an equirectangular projection that uses the
 *         cosine of the origin's latitude.
 *
 *         Over a track (a few hundred metres) the error against great-circle
 *         distance is far below GPS noise: under 0.01% at 1 km at Edinburgh's
 *         latitude. Only the origin subtraction is done in double; the
 *         results are float metres.
 ******************************************************************************/

#ifndef OVAL_LOCAL_FRAME_HPP
#define OVAL_LOCAL_FRAME_HPP

#include <cmath>

namespace Oval {

/** A point or vector in local metres: x east, y north. */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2  operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
inline Vec2  operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
inline Vec2  operator*(Vec2 a, float k) noexcept { return {a.x * k, a.y * k}; }
inline float dot(Vec2 a, Vec2 b) noexcept { return a.x * b.x + a.y * b.y; }
inline float length(Vec2 a) noexcept { return std::sqrt(dot(a, a)); }

class LocalFrame {
public:
    /** Mean Earth radius (IUGG), metres. */
    static constexpr double kEarthRadiusM = 6371008.8;

    void setOrigin(double latDeg, double lonDeg) noexcept;
    void clear() noexcept { mSet = false; }
    bool hasOrigin() const noexcept { return mSet; }

    double originLat() const noexcept { return mLat0; }
    double originLon() const noexcept { return mLon0; }

    /** Local metres of a point. Handles the 180 degree meridian. */
    Vec2 toLocal(double latDeg, double lonDeg) const noexcept;

    /** The inverse of toLocal(), for tests and the trace generator. */
    void toLatLon(Vec2 p, double& latDeg, double& lonDeg) const noexcept;

private:
    double mLat0       = 0.0;
    double mLon0       = 0.0;
    double mMPerDegLat = 0.0;
    double mMPerDegLon = 0.0;
    bool   mSet        = false;
};

} // namespace Oval

#endif // OVAL_LOCAL_FRAME_HPP
