/*******************************************************************************
 * @file   LocalFrame.cpp
 * @brief  Oval: equirectangular projection around an origin.
 ******************************************************************************/

#include "Oval/LocalFrame.hpp"

namespace Oval {

namespace {

constexpr double kDegToRad = 0.017453292519943295;

} // namespace

void LocalFrame::setOrigin(double latDeg, double lonDeg) noexcept
{
    mLat0       = latDeg;
    mLon0       = lonDeg;
    mMPerDegLat = kEarthRadiusM * kDegToRad;
    mMPerDegLon = mMPerDegLat * std::cos(latDeg * kDegToRad);
    mSet        = true;
}

Vec2 LocalFrame::toLocal(double latDeg, double lonDeg) const noexcept
{
    double dLon = lonDeg - mLon0;
    if (dLon > 180.0) {
        dLon -= 360.0;
    } else if (dLon < -180.0) {
        dLon += 360.0;
    }
    return {static_cast<float>(dLon * mMPerDegLon),
            static_cast<float>((latDeg - mLat0) * mMPerDegLat)};
}

void LocalFrame::toLatLon(Vec2 p, double& latDeg, double& lonDeg) const noexcept
{
    latDeg = mLat0 + static_cast<double>(p.y) / mMPerDegLat;
    lonDeg = mLon0 + static_cast<double>(p.x) / mMPerDegLon;
    if (lonDeg > 180.0) {
        lonDeg -= 360.0;
    } else if (lonDeg < -180.0) {
        lonDeg += 360.0;
    }
}

} // namespace Oval
