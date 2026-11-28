/*******************************************************************************
 * @file   Calibrator.hpp
 * @brief  Oval: fits a 400 m track template to the GPS trace (see
 *         docs/OVAL.md, calibration).
 *
 *         Good fixes, in the counting-mode local frame, are stored one every
 *         few metres travelled, each tagged with the lane the runner had set.
 *         Each time the points go once more round the track, the template is
 *         fitted: the centre, the heading of the straights, the kerb radius R
 *         and the runner's mean distance from their lane line, by robust
 *         least squares (Levenberg-Marquardt inside iteratively reweighted
 *         least squares with Huber weights).
 *
 *         Stages: provisional after one lap of coverage, refined with each
 *         lap after it, locked from lap 4 once successive fits agree. A fit
 *         that is not a standard track is rejected: shape mismatch, a high
 *         residual, a radius on the edge of its range, coverage missing, or
 *         fits that still disagree after calMaxLaps.
 *
 *         No heap, no exceptions, no SDK headers. The fit runs once a lap,
 *         inside the add() call that completes the lap.
 ******************************************************************************/

#ifndef OVAL_CALIBRATOR_HPP
#define OVAL_CALIBRATOR_HPP

#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/LocalFrame.hpp"
#include "Oval/TrackModel.hpp"

#include <cstdint>

namespace Oval {

class Calibrator {
public:
    static constexpr uint16_t kMaxPoints = 512;

    enum class Stage : uint8_t {
        Collecting,  ///< No accepted fit yet
        Provisional, ///< First fit, after one lap of coverage
        Refined,     ///< Refitted with each further lap
        Locked,      ///< Successive fits agree after calLockLaps: final
        Rejected,    ///< Not a standard track: final
    };

    enum class Reject : uint8_t {
        None,
        Shape,       ///< The trace does not have the template's shape
        Residual,    ///< Robust residual scale too high
        RadiusEdge,  ///< R at the edge of the searched range
        Coverage,    ///< Part of the fitted track has no points
        Disagree,    ///< Successive fits still disagree after calMaxLaps
    };

    /** What one add() changed. */
    enum class Change : uint8_t { None, Provisional, Refined, Locked, Rejected };

    /** The figures of the latest fit, accepted or not. */
    struct FitInfo {
        bool    valid      = false;
        uint8_t laps       = 0;      ///< Laps of coverage at the fit
        float   scaleM     = 0.0f;   ///< Robust residual scale (1.4826 x MAD)
        float   shapeM     = 0.0f;   ///< Noise-corrected systematic residual
        float   runnerM    = 0.0f;   ///< Runner's mean distance outside their lane line
        float   kerbRadiusM = 0.0f;
        float   centreMoveM = 0.0f;  ///< Centre moved since the previous fit
        float   headingMoveDeg = 0.0f;
        uint8_t emptyBins  = 0;      ///< Station bins short of minBinPoints
        uint8_t iterations = 0;
        Reject  failed     = Reject::None; ///< Quality check result
    };

    explicit Calibrator(const Config& cfg = Config()) noexcept;

    void setConfig(const Config& cfg) noexcept;

    /** Starts over with no points, for a run with this lane table. */
    void reset(const LaneTable& table) noexcept;

    /** A good fix in the local frame, with the runner's lane at the time, its
     *  altitude (NaN if none) and its HDOP (negative if none), which weights
     *  it in the fit (hdopWeight()). Ignored once locked or rejected. */
    Change add(Vec2 p, uint8_t lane, float altM, float hdop = -1.0f) noexcept;

    Stage   stage() const noexcept { return mStage; }
    Reject  rejectReason() const noexcept { return mReject; }
    bool    hasModel() const noexcept { return mStage == Stage::Provisional || mStage == Stage::Refined
                                               || mStage == Stage::Locked; }
    bool    isFinal() const noexcept { return mStage == Stage::Locked || mStage == Stage::Rejected; }

    /** Laps of coverage: times the points have gone all the way round. */
    uint8_t lapsCovered() const noexcept { return mLaps; }

    /** The accepted model (valid once provisional). Its latitude and
     *  longitude are not set: the caller knows the frame. */
    const TrackModel& model() const noexcept { return mModel; }

    const FitInfo& lastFit() const noexcept { return mFit; }

    uint16_t pointCount() const noexcept { return mCount; }
    float    spacingM() const noexcept { return mSpacingM; }

    /** Stored point i (0 to pointCount() - 1) in the local frame, and the
     *  lane it was tagged with. For aligning a stored track. */
    Vec2    point(uint16_t i) const noexcept { return pointAt(i); }
    uint8_t pointLane(uint16_t i) const noexcept { return mPts[i].lane; }
    /** Its weight from its HDOP (hdopWeight(); 1 with none). */
    float   pointWeight(uint16_t i) const noexcept;
    /** Its HDOP (to 0.05), or -1 with none. */
    float   pointHdop(uint16_t i) const noexcept;

    /** The robust residual scale (as FitInfo::scaleM) of the stored points
     *  against another model, with the runner this far outside their line.
     *  For the visit check: how well a stored track fits this run. */
    float scaleFor(const TrackModel& m, float runnerM) const noexcept;

private:
    /** A stored point: decimetres in the local frame, the lane, and the HDOP
     *  in twentieths (0: none). */
    struct Point {
        int16_t x;
        int16_t y;
        uint8_t lane;
        uint8_t hdop20;
    };

    struct Params {
        float cx, cy, heading, kerbR, runner;
    };

    Vec2  pointAt(uint16_t i) const noexcept;
    void  thin() noexcept;
    void  updateCoverage() noexcept;
    bool  fit(Params& p, uint8_t& iterations) const noexcept;
    float residualScale(const Params& p) const noexcept;
    void  assess(const Params& p, FitInfo& info) const noexcept;
    Change runFit() noexcept;

    Config    mCfg;
    LaneTable mTable;

    Point    mPts[kMaxPoints];
    uint16_t mCount    = 0;
    float    mSpacingM = 3.0f;
    bool     mHaveLast = false;
    Vec2     mLast;

    // Coverage: angle swept around the centroid of the points
    uint16_t mCovChecked = 0;   ///< Points when the coverage was last worked out
    uint8_t  mLaps       = 0;
    bool     mClockwise  = false;

    double   mAltSum   = 0.0;
    uint32_t mAltCount = 0;

    Stage      mStage  = Stage::Collecting;
    Reject     mReject = Reject::None;
    bool       mHavePrev = false;
    Params     mPrev  = {};
    TrackModel mModel;
    FitInfo    mFit;
};

} // namespace Oval

#endif // OVAL_CALIBRATOR_HPP
