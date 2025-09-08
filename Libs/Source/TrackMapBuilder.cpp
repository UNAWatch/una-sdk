/**
 ******************************************************************************
 * @file    TrackMapBuilder.cpp
 * @date    12-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Creates a simple map of the activity track to display on the screen.
 ******************************************************************************
 */

#define _USE_MATH_DEFINES
#include <cmath>

#include "SDK/TrackMapBuilder.hpp"


TrackMapBuilder::TrackMapBuilder(size_t expectedPointsNum)
{
    mGpsPoints.reserve(expectedPointsNum);
}

bool TrackMapBuilder::useRelativeCoordinates(GpsPoint basePoint)
{
    if (!basePoint.valid()) {
        return false;
    }
    mBasePoint = basePoint;
    mUseRelative = true;
    return true;
}

bool TrackMapBuilder::setDistanceThreshold(const GpsPoint &basePoint,
        float minDistance)
{
    if (!basePoint.valid() || minDistance < 0) {
        return false;
    }
    mThresholds = calculateThresholds(basePoint, minDistance);
    return true;
}

bool TrackMapBuilder::addPoint(const GpsPoint &p)
{
    if (!p.valid()) {
        return false;
    }

    if (mGpsPoints.empty()) {
        mGpsPoints.push_back(p);

        // Initialize limits
        mMax = p;
        mMin = p;
        mMaxDistanceFromCenter = 0.0f;

    } else {
        // Skipping points that are close to each other
        GpsPoint dif = p - mGpsPoints.back();
        float dLat = std::abs(dif.latitude);
        float dlon = std::abs(dif.longitude);

        if (dLat > mThresholds.latitude || dlon > mThresholds.longitude) {
            mGpsPoints.push_back(p);

            // Immediately calculate the minimum and maximum coordinates for
            // centering the map, and the maximum deviation from the center for
            // determining the scale. This will avoid an additional pass
            // through the array when creating the map.
            updateRange(p, mMin, mMax, mMaxDistanceFromCenter);
        }
    }

    return true;
}

TrackMapScreen TrackMapBuilder::build(uint8_t radiusPx)
{
    return std::move(buildFromRaw(mGpsPoints.data(), mGpsPoints.size(), radiusPx,
            mMin, mMax, mMaxDistanceFromCenter));
}

TrackMapScreen TrackMapBuilder::build(const GpsPoint *points, size_t pointsNum, uint8_t radiusPx)
{
    if (points == nullptr || pointsNum < 2 || radiusPx == 0) {
        return {};
    }

    // Initialize limits
    GpsPoint min = points[0];
    GpsPoint max = points[0];
    float maxDist = 0.0f;

    for (size_t i = 0; i < pointsNum; i++) {
        if (!points[i].valid()) {
            continue;   // Skip invalid points
        }
        updateRange(points[i], min, max, maxDist);
    }

    return std::move(buildFromRaw(points, pointsNum, radiusPx, min, max, maxDist));
}

void TrackMapBuilder::rotateMap(TrackMapScreen &map, float angleDegrees)
{
    // Get center from existing track
    uint8_t centerX = (map.maxx + map.minx) / 2;
    uint8_t centerY = (map.maxy + map.miny) / 2;

    // Reset limits
    map.minx = 255;
    map.miny = 255;
    map.maxx = 0;
    map.maxy = 0;

    // Convert angle to radians
    float angleRad = toRadians(angleDegrees);

    // Calculate trigonometric values
    float cosTheta = std::cos(angleRad);
    float sinTheta = std::sin(angleRad);

    // Loop through all points in the vector
    for (auto &point : map.points) {
        // Offset a point relative to the center
        int16_t relativeX = point.x - centerX;
        int16_t relativeY = point.y - centerY;

        // Calculate new coordinates
        int16_t rotatedX = static_cast<int16_t>(relativeX * cosTheta
                - relativeY * sinTheta);
        int16_t rotatedY = static_cast<int16_t>(relativeX * sinTheta
                + relativeY * cosTheta);

        // Update point (move back relative to center)
        point.x = static_cast<uint8_t>(rotatedX + centerX);
        point.y = static_cast<uint8_t>(rotatedY + centerY);

        map.minx = std::min(map.minx, point.x);
        map.maxx = std::max(map.maxx, point.x);

        map.miny = std::min(map.miny, point.y);
        map.maxy = std::max(map.maxy, point.y);
    }
}

TrackMapBuilder::GpsPoint TrackMapBuilder::calculateThresholds(const GpsPoint &basePoint,
        float minDistance) const
{
    GpsPoint thresholds { };
    thresholds.latitude = minDistance / kMetersPerDegree;
    thresholds.longitude = minDistance / metersPerDegreeLon(basePoint);

    return thresholds;
}

void TrackMapBuilder::updateRange(const GpsPoint &p, GpsPoint &min,
        GpsPoint &max, float &maxDistance) const
{
    if (max.latitude < p.latitude) {
        max.latitude = p.latitude;
    }

    if (min.latitude > p.latitude) {
        min.latitude = p.latitude;
    }

    if (max.longitude < p.longitude) {
        max.longitude = p.longitude;
    }

    if (min.longitude > p.longitude) {
        min.longitude = p.longitude;
    }

    GpsPoint center = getCenter(min, max);
    GpsPoint real = p;

    // Find the largest deviation from the center
    // NOTE: we have to take into account real coordinates since longitude
    // depends on latitude, so with relative coordinates the maximum distance
    // (and final scale) will be incorrect
    if (mUseRelative) {
        center += mBasePoint;
        real += mBasePoint;
    }

    float dist = distance(center, real);
    if (maxDistance < dist) {
        maxDistance = dist;
    }
}

float TrackMapBuilder::toRadians(float degrees) const
{
    // Conversion factor from degrees to radians.
    return static_cast<float>((degrees * M_PI) / 180.0);
}

#if 0 // Use for debug
// This method uses a Haversine formula.
float TrackMapBuilder::distance(const GpsPoint &p1, const GpsPoint &p2) const
{
    float dLat = toRadians(p2.latitude - p1.latitude);
    float dLon = toRadians(p2.longitude - p1.longitude);

    float lat1 = toRadians(p1.latitude);
    float lat2 = toRadians(p2.latitude);

    float a = std::sin(dLat / 2) * std::sin(dLat / 2)
            + std::cos(lat1) * std::cos(lat2) * std::sin(dLon / 2)
              * std::sin(dLon / 2);
    float c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return kEarthRadius * c;
}
#else

// This method uses a simplified (Euclidean) flat-earth formula assuming
// small distances, which is appropriate for local map rendering purposes.
float TrackMapBuilder::distance(const GpsPoint &p1, const GpsPoint &p2) const
{
    float dx = (p2.longitude - p1.longitude)
            * std::cos(toRadians((p1.latitude + p2.latitude) * 0.5f));
    float dy = (p2.latitude - p1.latitude);
    return kMetersPerDegree * std::hypot(dx, dy);
}
#endif

TrackMapBuilder::GpsPoint TrackMapBuilder::getCenter(const GpsPoint &p1,
        const GpsPoint &p2) const
{
    GpsPoint center = p1 + p2;
    center.scale(0.5f, 0.5f);
    return center;
}

float TrackMapBuilder::getScale(float distance, uint8_t pixels) const
{
    return (distance > 0.0f) ? (pixels / distance) : 1.0f;
}

float TrackMapBuilder::metersPerDegreeLon(const GpsPoint &p) const
{
    return kMetersPerDegree * static_cast<float>(std::cos(toRadians(p.latitude)));
}

TrackMapScreen TrackMapBuilder::buildFromRaw(
        const GpsPoint *points, size_t pointsNum, uint8_t radiusPx,
        const GpsPoint &min, const GpsPoint &max, const float &maxDistance)
{
    TrackMapScreen map {};
    map.points.reserve(pointsNum);

    // Track Center
    GpsPoint center = getCenter(min, max);

    // Find the scale factor meters -> pixels
    float scale = getScale(maxDistance, radiusPx);

    // Initializing with Invalid Values
    uint8_t prevX = 255;
    uint8_t prevY = 255;
 
    map.minx = 255;
    map.miny = 255;
    map.maxx = 0;
    map.maxy = 0;

    // Passing Through Points and Adding Unique Pixels
    for (size_t i = 0; i < pointsNum; i++) {
        GpsPoint point = points[i]; // modifiable value
        if (!point.valid()) {
            continue;   // Skip invalid points
        }

        point -= center;
        point.scale(scale * kMetersPerDegree,
                scale * metersPerDegreeLon(
                        mUseRelative ? (mBasePoint + center) : center));

        // Make sure that the values do not exceed the limits of the uint8_t type
        if (point.longitude + radiusPx > 255 ||
                point.latitude + radiusPx > 255)
        {
            continue; // Skip invalid points
        }

        // Converting Coordinates to Pixels
        uint8_t pixelX = static_cast<uint8_t>(point.longitude + radiusPx);
        uint8_t pixelY = static_cast<uint8_t>(radiusPx - point.latitude);

        // A simple filter to skip points that are close to each other
        if (std::abs(pixelX - prevX) > 3 || std::abs(pixelY - prevY) > 3) {

            map.points.emplace_back(TrackMapScreen::Point { pixelX, pixelY });
            prevX = pixelX;
            prevY = pixelY;

            map.minx = std::min(map.minx, pixelX);
            map.maxx = std::max(map.maxx, pixelX);

            map.miny = std::min(map.miny, pixelY);
            map.maxy = std::max(map.maxy, pixelY);
        }
    }

    return map;
}
