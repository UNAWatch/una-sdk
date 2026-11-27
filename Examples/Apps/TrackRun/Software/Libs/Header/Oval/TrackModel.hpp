/*******************************************************************************
 * @file   TrackModel.hpp
 * @brief  Oval: a fitted 400 m track, and the projection of a point onto the
 *         measurement line of a lane.
 *
 *         The track is two straights joined by two semicircular bends. The
 *         kerb radius R may be 35 to 38 m; the straights then adjust so that
 *         lane 1 (0.30 m out from the kerb) is exactly 400 m:
 *             straight = (400 - 2 pi (R + 0.30)) / 2.
 *         A lane's measurement line is the same shape at radius R + offset,
 *         so its lap is 400 + 2 pi (offset - 0.30), as in LaneTable.
 *
 *         Track coordinates: u along the straights (the heading), v across.
 *         A station tau in [0, 4) names a place on the track the same way in
 *         every lane: 0 to 1 the first straight, 1 to 2 the first bend, 2 to 3
 *         the second straight, 3 to 4 the second bend, each as a fraction, in
 *         the running direction. tau 0 is the start of the straight at
 *         negative v (for anticlockwise running; mirrored for clockwise). The
 *         heading is kept in [0, 180) degrees, so the same track always gets
 *         the same stations.
 *
 *         No heap, no exceptions, no SDK headers.
 ******************************************************************************/

#ifndef OVAL_TRACK_MODEL_HPP
#define OVAL_TRACK_MODEL_HPP

#include "Oval/LocalFrame.hpp"

#include <cstdint>
#include <limits>

namespace Oval {

class TrackModel {
public:
    static constexpr float kMinRadiusM  = 35.0f;
    static constexpr float kMaxRadiusM  = 38.0f;
    static constexpr float kLane1LapM   = 400.0f;
    static constexpr float kLane1LineM  = 0.30f;

    /** Where a point lies against a lane's measurement line. */
    struct Projection {
        float s        = 0.0f; ///< Metres along the line from station 0, in [0, lap length)
        float lateralM = 0.0f; ///< Signed distance from the line, positive outward
        float tau      = 0.0f; ///< Station, in [0, 4)
    };

    TrackModel() noexcept = default;

    /** Sets the geometry in the local frame. headingRad is the direction of
     *  the straights (either way along them), kerbRadiusM the bend radius at
     *  the kerb. Clears the stored latitude and longitude. */
    void set(Vec2 centre, float headingRad, float kerbRadiusM, bool clockwise) noexcept;

    bool  isValid() const noexcept { return mValid; }
    void  clear() noexcept { *this = TrackModel(); }

    Vec2  centre() const noexcept { return mCentre; }
    float headingRad() const noexcept { return mHeading; }
    float headingDeg() const noexcept;
    float kerbRadiusM() const noexcept { return mKerbR; }
    bool  clockwise() const noexcept { return mClockwise; }

    /** Length of each straight, from the kerb radius. */
    float straightM() const noexcept;

    /** Lap length along a line this far out from the kerb. */
    float lapLengthM(float lineOffsetM) const noexcept;

    /** A point onto the line lineOffsetM out from the kerb. */
    Projection project(Vec2 p, float lineOffsetM) const noexcept;

    /** Metres along a line at a station, and the inverse. */
    float sOf(float tau, float lineOffsetM) const noexcept;
    float tauOf(float s, float lineOffsetM) const noexcept;

    /** The point at a station, lateralM out from the line. */
    Vec2 pointAt(float tau, float lineOffsetM, float lateralM = 0.0f) const noexcept;

    /** The outward unit normal at a station. */
    Vec2 normalAt(float tau) const noexcept;

    // Stored with the model, for stored tracks and the service ------------

    /** Mean GPS altitude of the fixes used (NaN if none had one). */
    float altitudeM() const noexcept { return mAltM; }
    void  setAltitudeM(float altM) noexcept { mAltM = altM; }

    /** The centre's latitude and longitude, from the frame it was fitted in. */
    void   setGeo(const LocalFrame& frame) noexcept;
    bool   hasGeo() const noexcept { return mHasGeo; }
    double centreLat() const noexcept { return mLat; }
    double centreLon() const noexcept { return mLon; }

    /** Places a stored model in another run's frame: the local centre is
     *  recomputed from the stored latitude and longitude. False without them. */
    bool placeIn(const LocalFrame& frame) noexcept;

    /** The same track run the other way round: the same geometry, latitude
     *  and longitude, with the stations reversed (a stored track run
     *  in the other direction). */
    TrackModel reversed() const noexcept;

    /** A model from stored values, for a supplied (stored) or test model. */
    static TrackModel fromGeo(double centreLat, double centreLon, float headingDeg,
                              float kerbRadiusM, bool clockwise, float altM) noexcept;

private:
    /** Track coordinates of a point, v mirrored for clockwise running. */
    void toTrack(Vec2 p, float& u, float& v) const noexcept;

    bool   mValid     = false;
    bool   mClockwise = false;
    bool   mHasGeo    = false;
    Vec2   mCentre;
    float  mHeading   = 0.0f; ///< Radians, [0, pi)
    float  mCos       = 1.0f;
    float  mSin       = 0.0f;
    float  mKerbR     = 36.5f;
    float  mAltM      = std::numeric_limits<float>::quiet_NaN();
    double mLat       = 0.0;
    double mLon       = 0.0;
};

} // namespace Oval

#endif // OVAL_TRACK_MODEL_HPP
