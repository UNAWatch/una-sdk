/*******************************************************************************
 * @file   TrackModel.cpp
 * @brief  Oval: track geometry and projection. See TrackModel.hpp.
 ******************************************************************************/

#include "Oval/TrackModel.hpp"

#include <cmath>

namespace Oval {

namespace {

constexpr float  kPi     = 3.14159265358979f;
constexpr double kPiD    = 3.141592653589793;
constexpr float  kRadDeg = 57.2957795130823f;

} // namespace

void TrackModel::set(Vec2 centre, float headingRad, float kerbRadiusM, bool clockwise) noexcept
{
    // The stadium looks the same turned half a circle, so keep the heading in
    // [0, pi): the same track then always gets the same stations.
    float h = std::fmod(headingRad, kPi);
    if (h < 0.0f) {
        h += kPi;
    }
    if (h >= kPi) {
        h = 0.0f;
    }
    mValid     = true;
    mCentre    = centre;
    mHeading   = h;
    mCos       = std::cos(h);
    mSin       = std::sin(h);
    mKerbR     = kerbRadiusM;
    mClockwise = clockwise;
    mHasGeo    = false;
    mLat       = 0.0;
    mLon       = 0.0;
}

float TrackModel::headingDeg() const noexcept
{
    return mHeading * kRadDeg;
}

float TrackModel::straightM() const noexcept
{
    return static_cast<float>((kLane1LapM - 2.0 * kPiD * (static_cast<double>(mKerbR) + kLane1LineM)) / 2.0);
}

float TrackModel::lapLengthM(float lineOffsetM) const noexcept
{
    // 2 x straight + 2 pi (R + offset), in double so every lane matches the
    // lane table to well under a centimetre.
    const double straight = (kLane1LapM - 2.0 * kPiD * (static_cast<double>(mKerbR) + kLane1LineM)) / 2.0;
    return static_cast<float>(2.0 * straight
                              + 2.0 * kPiD * (static_cast<double>(mKerbR) + static_cast<double>(lineOffsetM)));
}

void TrackModel::toTrack(Vec2 p, float& u, float& v) const noexcept
{
    const Vec2 d = p - mCentre;
    u = d.x * mCos + d.y * mSin;
    v = -d.x * mSin + d.y * mCos;
    if (mClockwise) {
        v = -v;
    }
}

TrackModel::Projection TrackModel::project(Vec2 p, float lineOffsetM) const noexcept
{
    float u = 0.0f, v = 0.0f;
    toTrack(p, u, v);
    const float straight = straightM();
    const float hs       = 0.5f * straight;
    const float rho      = mKerbR + lineOffsetM;

    Projection r;
    float seg = 0.0f, q = 0.0f;
    if (u >= -hs && u <= hs) {
        if (v < 0.0f) {
            seg = 0.0f;
            q   = (u + hs) / straight;
            r.lateralM = -v - rho;
        } else {
            seg = 2.0f;
            q   = (hs - u) / straight;
            r.lateralM = v - rho;
        }
    } else if (u > hs) {
        const float du = u - hs;
        seg = 1.0f;
        q   = std::atan2(du, -v) / kPi;
        r.lateralM = std::sqrt(du * du + v * v) - rho;
    } else {
        const float du = -hs - u;
        seg = 3.0f;
        q   = std::atan2(du, v) / kPi;
        r.lateralM = std::sqrt(du * du + v * v) - rho;
    }
    q = q < 0.0f ? 0.0f : (q > 1.0f ? 1.0f : q);
    r.tau = seg + q;
    if (r.tau >= 4.0f) {
        r.tau = 0.0f;
    }
    r.s = sOf(r.tau, lineOffsetM);
    return r;
}

float TrackModel::sOf(float tau, float lineOffsetM) const noexcept
{
    const float straight = straightM();
    const float bend     = kPi * (mKerbR + lineOffsetM);
    float t = std::fmod(tau, 4.0f);
    if (t < 0.0f) {
        t += 4.0f;
    }
    int seg = static_cast<int>(t);
    if (seg > 3) {
        seg = 3;
    }
    const float q = t - static_cast<float>(seg);
    switch (seg) {
    case 0:  return q * straight;
    case 1:  return straight + q * bend;
    case 2:  return straight + bend + q * straight;
    default: return 2.0f * straight + bend + q * bend;
    }
}

float TrackModel::tauOf(float s, float lineOffsetM) const noexcept
{
    const float straight = straightM();
    const float bend     = kPi * (mKerbR + lineOffsetM);
    const float lap      = 2.0f * (straight + bend);
    float a = std::fmod(s, lap);
    if (a < 0.0f) {
        a += lap;
    }
    const float lens[4] = {straight, bend, straight, bend};
    for (int i = 0; i < 4; ++i) {
        if (a < lens[i] || i == 3) {
            float q = a / lens[i];
            if (q >= 1.0f) {
                q = 0.9999999f;
            }
            return static_cast<float>(i) + q;
        }
        a -= lens[i];
    }
    return 0.0f;
}

Vec2 TrackModel::pointAt(float tau, float lineOffsetM, float lateralM) const noexcept
{
    const float hs = 0.5f * straightM();
    const float r  = mKerbR + lineOffsetM + lateralM;
    float t = std::fmod(tau, 4.0f);
    if (t < 0.0f) {
        t += 4.0f;
    }
    int seg = static_cast<int>(t);
    if (seg > 3) {
        seg = 3;
    }
    const float q = t - static_cast<float>(seg);
    float u = 0.0f, v = 0.0f;
    switch (seg) {
    case 0:  u = -hs + q * 2.0f * hs; v = -r; break;
    case 1:  u = hs + r * std::sin(q * kPi); v = -r * std::cos(q * kPi); break;
    case 2:  u = hs - q * 2.0f * hs; v = r; break;
    default: u = -hs - r * std::sin(q * kPi); v = r * std::cos(q * kPi); break;
    }
    if (mClockwise) {
        v = -v;
    }
    return {mCentre.x + u * mCos - v * mSin, mCentre.y + u * mSin + v * mCos};
}

Vec2 TrackModel::normalAt(float tau) const noexcept
{
    float t = std::fmod(tau, 4.0f);
    if (t < 0.0f) {
        t += 4.0f;
    }
    int seg = static_cast<int>(t);
    if (seg > 3) {
        seg = 3;
    }
    const float q = t - static_cast<float>(seg);
    float u = 0.0f, v = 0.0f;
    switch (seg) {
    case 0:  v = -1.0f; break;
    case 1:  u = std::sin(q * kPi); v = -std::cos(q * kPi); break;
    case 2:  v = 1.0f; break;
    default: u = -std::sin(q * kPi); v = std::cos(q * kPi); break;
    }
    if (mClockwise) {
        v = -v;
    }
    return {u * mCos - v * mSin, u * mSin + v * mCos};
}

void TrackModel::setGeo(const LocalFrame& frame) noexcept
{
    if (!frame.hasOrigin()) {
        return;
    }
    frame.toLatLon(mCentre, mLat, mLon);
    mHasGeo = true;
}

bool TrackModel::placeIn(const LocalFrame& frame) noexcept
{
    if (!mHasGeo || !frame.hasOrigin()) {
        return false;
    }
    mCentre = frame.toLocal(mLat, mLon);
    return true;
}

TrackModel TrackModel::reversed() const noexcept
{
    // The stadium is symmetric across its long axis, so mirroring v keeps
    // every point where it is and only runs the stations the other way.
    TrackModel m = *this;
    m.mClockwise = !mClockwise;
    return m;
}

TrackModel TrackModel::fromGeo(double centreLat, double centreLon, float headingDeg,
                               float kerbRadiusM, bool clockwise, float altM) noexcept
{
    TrackModel m;
    m.set(Vec2(), headingDeg / kRadDeg, kerbRadiusM, clockwise);
    m.mLat    = centreLat;
    m.mLon    = centreLon;
    m.mHasGeo = true;
    m.mAltM   = altM;
    return m;
}

} // namespace Oval
