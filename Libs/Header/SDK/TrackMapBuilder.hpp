/**
 ******************************************************************************
 * @file    TrackMapBuilder.hpp
 * @date    12-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Creates a simple map of the activity track to display on the screen.
 ******************************************************************************
 */

#ifndef __TRACK_MAP_BUILDER_HPP
#define __TRACK_MAP_BUILDER_HPP

#include <cstdint>
#include <vector>
#include <memory>

#include "TrackMapScreen.hpp"

/**
 * @class TrackMapBuilder
 * @brief Builds a simplified map representation of GPS tracks for display purposes.
 */
class TrackMapBuilder {
public:

#pragma pack(push, 4)
    /**
     * @struct GpsPoint
     * @brief Represents a geographical point with latitude and longitude.
     */
    struct GpsPoint {
        float latitude;  ///< Latitude in degrees.
        float longitude; ///< Longitude in degrees.

        /**
         * @brief Checks if the GPS point has valid latitude and longitude.
         * @return True if valid, false otherwise.
         */
        bool valid() const
        {
            return latitude >= -90.0f && latitude <= 90.f && longitude >= -180.0f
                && longitude <= 180.0f;
        }

        /**
         * @brief Scales the latitude and longitude by given factors.
         * @param lat: Scaling factor for latitude.
         * @param lon: Scaling factor for longitude.
         */
        void scale(float lat, float lon)
        {
            latitude *= lat;
            longitude *= lon;
        }

        /**
         * @brief Adds two GpsPoint objects.
         * @param other: Another GpsPoint to add.
         * @return Resulting GpsPoint after addition.
         */
        GpsPoint operator+(const GpsPoint &other) const
        {
            return {latitude + other.latitude, longitude + other.longitude};
        }

        /**
         * @brief Subtracts another GpsPoint from this one.
         * @param other: Another GpsPoint to subtract.
         * @return Resulting GpsPoint after subtraction.
         */
        GpsPoint operator-(const GpsPoint &other) const
        {
            return {latitude - other.latitude, longitude - other.longitude};
        }

        /**
         * @brief Adds another GpsPoint to this one.
         * @param other Another GpsPoint to add.
         * @return Reference to this GpsPoint after addition.
         */
        GpsPoint &operator+=(const GpsPoint &other)
        {
            latitude += other.latitude;
            longitude += other.longitude;
            return *this;
        }

        /**
         * @brief Subtracts another GpsPoint from this one.
         * @param other Another GpsPoint to subtract.
         * @return Reference to this GpsPoint after subtraction.
         */
        GpsPoint &operator-=(const GpsPoint &other)
        {
            latitude -= other.latitude;
            longitude -= other.longitude;
            return *this;
        }

    };
#pragma pack(pop)

    /**
     * @brief Constructor for TrackMapBuilder.
     * @param expectedPointsNum: Expected number of GPS points to reserve space for.
     */
    TrackMapBuilder(size_t expectedPointsNum = 360); // For 1h with period of 10s

    /**
     * @brief Sets the base point for relative coordinate calculations.
     * @note Use this method before addPoint() for correct scaling.
     * @param basePoint: The base GPS point.
     * @return True if the base point is valid and set successfully.
     */
    bool useRelativeCoordinates(GpsPoint basePoint);

    /**
     * @brief Sets the minimum distance threshold for adding new points.
     * @note Use this method before addPoint() for filtering points.
     * @param basePoint: The base GPS point.
     * @param minDistance: Minimum distance in meters between points.
     * @return True if thresholds are set successfully.
     */
    bool setDistanceThreshold(const GpsPoint &basePoint, float minDistance);

    /**
     * @brief Adds a GPS point to the track if it meets the distance threshold.
     * @param p: The GPS point to add.
     * @return True if the point is valid and added successfully.
     */
    bool addPoint(const GpsPoint &p);

    /**
     * @brief Builds the screen map from the collected GPS points.
     * @note Use addPoint() to collect GPS points.
     * @param radiusPx: Radius in pixels for scaling.
     * @return TrackMapScreen.
     */
    TrackMapScreen build(uint8_t radiusPx);

    /**
     * @brief Builds the screen map from provided GPS points.
     * @param points: Array of GPS points.
     * @param pointsNum: Number of points in the array.
     * @param radiusPx: Radius in pixels for scaling.
     * @return TrackMapScreen.
     */
    TrackMapScreen build(const GpsPoint *points, size_t pointsNum, uint8_t radiusPx);

    /**
     * @brief Rotates the map around a center point by a given angle.
     * @param map: TrackMapScreen to rotate.
     * @param angleDegrees: Rotation angle in degrees.
     */
    void rotateMap(TrackMapScreen &map, float angleDegrees);

private:
    GpsPoint mMin { }; ///< Minimum GPS coordinates in the track.
    GpsPoint mMax { }; ///< Maximum GPS coordinates in the track.

    /// Thresholds for latitude and longitude differences.
    GpsPoint mThresholds { };

    /// Base point for relative coordinate calculations.
    GpsPoint mBasePoint { };

    /// Flag to indicate if relative coordinates are used.
    bool mUseRelative = false;

    /// Maximum distance from the center point.
    float mMaxDistanceFromCenter = 0.0f;

    /// Vector of collected GPS points.
    std::vector<GpsPoint> mGpsPoints;

    /// Approximate meters per degree of latitude.
    static constexpr float kMetersPerDegree = 111320.0f;

    /// Earth's radius in meters.
    static constexpr float kEarthRadius = 6371000.0f;


    /**
     * @brief Calculates latitude and longitude thresholds based on minimum distance.
     * @param basePoint: The base GPS point.
     * @param minDistance: Minimum distance in meters.
     * @return GpsPoint containing latitude and longitude thresholds.
     */
    GpsPoint calculateThresholds(const GpsPoint &basePoint,
            float minDistance) const;

    /**
     * @brief Updates the min, max, and maxDistance values based on a new point.
     * @param p: New GPS point.
     * @param min: Reference to current minimum GPS point.
     * @param max: Reference to current maximum GPS point.
     * @param maxDistance: Reference to current maximum distance from center.
     */
    void updateRange(const GpsPoint &p, GpsPoint &min, GpsPoint &max,
            float &maxDistance) const;

    /**
     * @brief Converts degrees to radians.
     * @param degrees Angle in degrees.
     * @return Angle in radians.
     */
    float toRadians(float degrees) const;

    /**
     * @brief Calculates the distance in meters between two GPS points.
     * @param p1: First GPS point.
     * @param p2: Second GPS point.
     * @return Approximate distance between the two points in meters.
     */
    float distance(const GpsPoint &p1, const GpsPoint &p2) const;

    /**
     * @brief Calculates the center point between two GPS coordinates.
     * @param min: Minimum GPS point (lower-left corner).
     * @param max: Maximum GPS point (upper-right corner).
     * @return GpsPoint representing the center between min and max.
     */
    GpsPoint getCenter(const GpsPoint &p1, const GpsPoint &p2) const;

    /**
     * @brief Computes the scale factor to convert meters to pixels.
     *
     * This is used to scale the physical distance into a pixel-based
     * coordinate system based on the available radius in pixels.
     *
     * @param distance: Distance in meters to represent.
     * @param pixels: Number of pixels available for rendering.
     * @return Scaling factor in pixels per meter.
     */
    float getScale(float distance, uint8_t pixels) const;

    /**
     * @brief Calculates meters per degree of longitude at a given latitude.
     *
     * Since the length of a degree of longitude changes with latitude,
     * this function corrects for the Earth's curvature.
     *
     * @param p: GPS point used to compute latitude-based correction.
     * @return Number of meters represented by one degree of longitude
     *         at the point.
     */
    float metersPerDegreeLon(const GpsPoint &p) const;

    /**
     * @brief Builds a screen map from raw GPS points.
     *
     * Transforms GPS coordinates into pixel coordinates relative to a center point.
     * Applies scaling and filters out repeated points within the same pixel.
     *
     * @param points: Pointer to array of GPS points.
     * @param pointsNum: Number of points in the array.
     * @param radiusPx: Radius in pixels used for scaling and bounds.
     * @param min: Minimum GPS point in the set.
     * @param max: Maximum GPS point in the set.
     * @param maxDistance: Maximum distance from the center used for scaling.
     * @return Vector of unique screen points.
     */
    TrackMapScreen buildFromRaw(
            const GpsPoint *points, size_t pointsNum, uint8_t radiusPx,
            const GpsPoint &min, const GpsPoint &max, const float &maxDistance);

};

#endif /* __TRACK_MAP_BUILDER_HPP */
