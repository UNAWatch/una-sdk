/*******************************************************************************
 * @file   Config.hpp
 * @brief  Oval: every tuning threshold, with its default: counting mode,
 *         then calibration, calibrated mode and lap timing, then stored
 *         tracks and return visits, then road and track in one run
 *         (Session). The GPS precision figure is HDOP,
 *         with the helpers that turn it into an error in metres and a
 *         weight.
 *
 *         docs/OVAL.md explains each one.
 ******************************************************************************/

#ifndef OVAL_CONFIG_HPP
#define OVAL_CONFIG_HPP

#include <cstdint>

namespace Oval {

struct Config {
    // Reading quality (the precision figure is HDOP) -------------------------

    /** Readings with a worse HDOP are not used for any position decision
     *  (start point, gate, crossings, the step-sum fallback, calibration and
     *  calibrated mode). Readings with no HDOP count as good. */
    float maxHdop = 4.0f;

    /** HDOP taken for a reading with none, the top of the normal range. Also
     *  the reference that readings are weighted against (hdopRelError()). */
    float unknownHdop = 1.5f;

    /** Metres of horizontal error per unit of HDOP (the user equivalent range
     *  error): hdopErrorM() is HDOP times this. */
    float uereM = 4.0f;

    // Start point -----------------------------------------------------------

    /** Primed fixes this recent at Start are averaged into the start point. */
    uint32_t primeWindowMs = 5000;

    /** If the first readings after Start (at least 3, while the runner is
     *  still within gateResetM of their mean) sit further than this from the
     *  primed start point, the primed fixes were stale, as from a fix still
     *  settling, and the start point moves to their mean. */
    float staleStartM = 15.0f;

    // Gate ------------------------------------------------------------------

    /** The gate line runs through the start point, across the direction
     *  from the start point to the runner once the runner is this far away. */
    float gateDirDistM = 20.0f;

    /** Consecutive good readings at gateDirDistM or more needed before that
     *  direction is taken. Their mean position gives the direction. */
    uint8_t gateDirConfirm = 2;

    /** A good reading back within this distance of the start point ends the
     *  excursion, and the next one sets the gate line again. */
    float gateResetM = 10.0f;

    /** A crossing is counted at once if it happens within this distance of
     *  the start point (the point on the line between the two readings). */
    float gateRadiusM = 25.0f;

    /** A crossing is counted at once only if its two readings are at most
     *  this far apart in time. A wider gap makes it a pending pass. */
    uint32_t maxCrossGapMs = 10000;

    /** A crossing of the gate line at least this far from the start point is
     *  on the far side of the track. The first one makes the gate final and
     *  sets which way it faces (the runner crosses there heading back).
     *  After that, a backward crossing there arms the gate for the next lap. */
    float armFarM = 40.0f;

    /** A forward crossing of the gate line within this distance of the start
     *  point that could not be counted at once (beyond gateRadiusM, a gap
     *  longer than maxCrossGapMs, or while paused) is held as a pending pass
     *  and credited later, timed at the crossing. A countable crossing soon
     *  after it (under minLapFrac on) replaces it. */
    float passRadiusM = 35.0f;

    // Guards ----------------------------------------------------------------

    /** A lap needs at least this fraction of a lap since the last lap, by the
     *  scaled GPS distance. */
    float minLapFrac = 0.70f;

    /** A lap needs at least this much active time since the last lap. The
     *  400 m world record is 43 s. */
    uint32_t minLapMs = 40000;

    /** With a pending pass, the missed lap is credited at this fraction of a
     *  lap since the last lap (or sooner, at the next far-side crossing).
     *  And a real crossing ending a lap at least this long, after a gap in
     *  the readings, first credits the lap that the gap hid. */
    float missedLapFrac = 1.5f;

    /** With no pending pass at all, a lap is credited by distance alone at
     *  this fraction of a lap since the last lap, but only if this lap had a
     *  gap in good readings longer than maxCrossGapMs (with readings every
     *  few seconds a pass of the gate is always seen). */
    float missedLapNoPassFrac = 2.5f;

    // Part-lap estimate -----------------------------------------------------

    /** Bounds on ratio = credited lap length / GPS distance of that lap. */
    float ratioMin = 0.80f;
    float ratioMax = 1.25f;

    /** The part-lap estimate never exceeds this fraction of the lap. */
    float partLapCap = 0.98f;

    /** A step of GPS distance faster than this is cut to this speed. */
    float maxSpeedMps = 12.0f;

    // Calibration -----------------------------------------------------------

    /** A point is stored once the runner is this far from the last stored
     *  point, so slow and fast runners give similar data. Doubles each time
     *  the buffer is thinned. */
    float calSpacingM = 3.0f;

    /** Laps of coverage for the provisional fit, for locking, and the most
     *  allowed before a fit that still has not locked counts as successive
     *  fits disagreeing. A fit failing the quality check before
     *  calRejectLaps waits for more data; from then on it rejects. */
    uint8_t calProvisionalLaps = 1;
    uint8_t calLockLaps        = 4;
    uint8_t calMaxLaps         = 6;
    uint8_t calRejectLaps      = 3;

    /** Successive fits agree when their centres and headings are this close. */
    float lockCentreM   = 1.5f;
    float lockHeadingDeg = 1.5f;

    /** The fit searches bend radii from fitRadiusMinM to fitRadiusMaxM, wider
     *  than the allowed 35 to 38 m so a real 35 m or 38 m track is not cut off
     *  by noise. A fit that ends on the edge of this range is not a standard
     *  track. The four non-standard shapes all fit large radii, so only the
     *  top edge is tight. */
    float fitRadiusMinM = 32.5f;
    float fitRadiusMaxM = 39.75f;

    /** Quality: the noise-corrected systematic residual (shape mismatch) and
     *  the robust residual scale, each at most this. */
    float maxShapeM = 2.0f;
    float maxScaleM = 9.0f;

    /** Coverage: each twelfth of the fitted track needs this many
     *  inlier points. */
    uint8_t minBinPoints = 2;

    // Calibrated mode -------------------------------------------------------

    /** Alpha-beta filter on the along-line position. */
    float abAlpha = 0.5f;
    float abBeta  = 0.12f;

    /** A reading whose along-line position is further than this from the
     *  prediction (widened by HDOP, plus half of maxSpeedMps x the time since
     *  the last reading used) is an outlier. Three in a row that agree restart the filter there. */
    float outlierM = 20.0f;

    /** Progress is credited only while the smoothed speed along the line is
     *  at least this: standing near the line, noise alone never moves the
     *  distance on or ends a lap. Crediting is only delayed, never lost. */
    float stillMps = 0.5f;

    /** Off track: offTrackReadings readings in a row beyond the lanes by
     *  offLanesM (below) freeze progress; offTrackMs of it drops to counting
     *  mode; back within three quarters of offLanesM for rejoinReadings
     *  readings rejoins. Where the runner left is the last reading within a
     *  third of offTrackM of their own line that still moved along it. */
    float    offTrackM        = 15.0f;

    /** Leaving the track is judged outside its lanes, from lane 1's line to
     *  the last lane's, not from the runner's lane line: a lane set that is
     *  not the runner's is still the track. Off once further than offLanesM
     *  beyond them; back on within three quarters of it. 12 m beyond the
     *  lanes is about 16 m from a middle lane's line. */
    float    offLanesM        = 12.0f;
    uint8_t  offTrackReadings = 3;
    uint32_t offTrackMs       = 30000;
    uint8_t  rejoinReadings   = 5;

    /** The switch from counting to calibrated mode mid-run waits until the
     *  runner is this far round the lap from the start line (as fractions),
     *  so which lap they are on cannot be in doubt. */
    float switchFromFrac = 0.25f;
    float switchToFrac   = 0.75f;

    /** Drift tracking, once the model is locked or supplied: the offset
     *  between GPS and the model moves at most driftPerLapM a lap. */
    float driftPerLapM = 1.0f;

    // Lap timing --------------------------------------------------------------

    /** The refined lap time fits a line through the readings from
     *  refineBeforeMs before the crossing to refineAfterMs after it. */
    uint32_t refineBeforeMs = 5000;
    uint32_t refineAfterMs  = 3000;

    // Stored tracks ---------------------------------------------------------

    /** A stored track is recognised at Start when its centre is this close
     *  to the averaged pre-run position. */
    float storeNearM = 150.0f;

    /** A wrongly picked stored track is replaced by the run's own locked fit
     *  when the two centres are this close; further apart, the fit is saved
     *  as a new track. */
    float storeReplaceNearM = 50.0f;

    /** When a visit agrees with the stored track it is blended in, weighted
     *  by the visits so far, up to this many: a visit always counts at least
     *  1 / (weight + 1). */
    uint8_t storeBlendMaxWeight = 9;

    // Return visits: lining a stored track up with today's GPS --------------

    /** The day's offset between GPS and the stored track is fitted freely
     *  (no driftPerLapM limit) until the runner has gone at least alignMinM
     *  along the track, the points seen face enough directions (the smaller
     *  eigenvalue of the sum of n n^T at least alignCondFrac of its trace:
     *  roughly the first bend) and no direction of the offset is undecided
     *  (between alignSigLo and alignSigHi); in any case by alignMaxM, before
     *  lap 1 can end. Then drift tracking takes over. Points further than
     *  alignHuberM from the line are down-weighted. */
    float alignMinM     = 100.0f;
    float alignMaxM     = 350.0f;
    float alignCondFrac = 0.2f;
    float alignHuberM   = 5.0f;

    /** From alignMinM on, an offset in use beyond alignMaxOffsetM, or a
     *  robust residual scale beyond alignMaxScaleM, means the stored track is
     *  not this one (a wrong pick): it is dropped and the run calibrates from
     *  scratch. */
    float alignMaxOffsetM = 35.0f;
    float alignMaxScaleM  = 9.0f;

    /** Running direction: while lining up, a runner whose own movement has
     *  gone this far against the stored direction along the track is running
     *  it the other way round, and the model is reversed. Not once lap 1
     *  progress has reached alignFlipMaxM. */
    float alignDirM     = 30.0f;
    float alignFlipMaxM = 100.0f;

    /** While lining up, each direction of the offset estimate is used in
     *  full beyond alignSigHi standard errors, not at all within alignSigLo,
     *  and in proportion between: a day's offset of several metres is used
     *  whole, and a track stored in the right place is not moved by noise. */
    float alignSigLo = 3.5f;
    float alignSigHi = 5.5f;

    // Checking a stored track on each visit ---------------------------------

    /** The visit's locked fit agrees with the stored track (after removing
     *  the day's offset) when the heading, R and centre are within these and
     *  the stored track fits the visit's points at most visitAgreeScaleRatio
     *  times worse than the visit's own fit. It clearly disagrees when any is
     *  beyond the visitReplace values. In between, the stored track stays. */
    float visitAgreeHeadingDeg   = 2.0f;
    float visitAgreeRM           = 1.0f;
    float visitAgreeCentreM      = 3.0f;
    float visitAgreeScaleRatio   = 1.25f;
    float visitReplaceHeadingDeg = 4.0f;
    float visitReplaceRM         = 2.0f;
    float visitReplaceCentreM    = 6.0f;
    float visitReplaceScaleRatio = 1.5f;

    // Road and track in one run (Oval::Session) -----------------------------

    /** Arriving at a known track (stored, or visited earlier in the run):
     *  readings within joinMarginM of the track (inside the kerb, or outside
     *  lane 8's outer edge), moving along it, for joinConfirmM of path. The
     *  net movement along the track must be at least joinAlongFrac of the
     *  path. After a join that is undone, none for joinRetryM of path. */
    float joinMarginM   = 6.0f;
    float joinConfirmM  = 40.0f;
    float joinAlongFrac = 0.7f;
    float joinRetryM    = 200.0f;

    /** Leaving a track: confirmed after offTrackMs off it, or sooner once
     *  offTrackReadings readings in a row are further than exitFarM beyond
     *  the lanes (no infield cut goes that far). With no fitted track
     *  (counting mode), the runner has left once further than exitCountingM
     *  from where the visit began; the way back to exitAwayM from it was the
     *  road. */
    float exitFarM      = 50.0f;
    float exitAwayM     = 190.0f;
    float exitCountingM = 250.0f;

    /** The Session keeps a point every ringSpacingM of GPS distance, for
     *  replaying a join it notices. */
    float ringSpacingM = 3.0f;

    /** Road splits: one each time road distance passes another whole
     *  roadSplitM in the road segment. */
    float roadSplitM = 1000.0f;
};

/** Estimated horizontal error in metres of a reading with this HDOP: HDOP
 *  times uereM. A negative HDOP (unknown) is taken as unknownHdop. */
inline float hdopErrorM(float hdop, const Config& c) noexcept
{
    return (hdop >= 0.0f ? hdop : c.unknownHdop) * c.uereM;
}

/** A reading's estimated error relative to that of a reading with no HDOP:
 *  exactly 1 with no HDOP, so such a run is weighted as if HDOP were not used.
 *  HDOP is taken as at least a third of unknownHdop here. */
inline float hdopRelError(float hdop, const Config& c) noexcept
{
    const float ref = hdopErrorM(-1.0f, c);
    if (!(hdop >= 0.0f) || !(ref > 0.0f)) {
        return 1.0f;
    }
    const float lo = c.unknownHdop / 3.0f;
    return hdopErrorM(hdop > lo ? hdop : lo, c) / ref;
}

/** A reading's weight wherever readings are weighted (calibration, lining up
 *  a stored track, drift tracking): 1 / its relative error squared. 1 with
 *  no HDOP; at most 9. */
inline float hdopWeight(float hdop, const Config& c) noexcept
{
    const float k = hdopRelError(hdop, c);
    return 1.0f / (k * k);
}

/** How far the distances a reading is judged by (off the track, an outlier
 *  along it) widen: its relative error, but never below 1, so they only
 *  ever widen for readings poorer than unknownHdop. */
inline float hdopWiden(float hdop, const Config& c) noexcept
{
    const float k = hdopRelError(hdop, c);
    return k > 1.0f ? k : 1.0f;
}

} // namespace Oval

#endif // OVAL_CONFIG_HPP
