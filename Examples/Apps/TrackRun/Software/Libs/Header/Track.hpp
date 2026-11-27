/**
 ******************************************************************************
 * @file    Track.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Track namespace: state, interval types and real-time metrics.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef TRACK_HPP
#define TRACK_HPP

#include <cstdint>
#include <ctime>

namespace Track
{

/**
 * @brief Activity tracking state.
 */
enum class State {
    INACTIVE = 0,
    ACTIVE,
    PAUSED
};

// =============================================================================
// Interval training types
// =============================================================================

/**
 * @brief Current phase of an interval training session.
 */
enum class IntervalsPhase : uint8_t {
    WARM_UP   = 0,
    RUN,
    REST,
    COOL_DOWN
};

/**
 * @brief Timer display mode for the current interval phase.
 *
 * Determined by the workout configuration; the Service sets this field so
 * the GUI never needs to deduce it.
 */
enum class IntervalsMetric : uint8_t {
    TIME_OPEN       = 0, ///< Count-up, no fixed target     — label "Open"  (default / WARM_UP / COOL_DOWN)
    TIME_REMAINING,      ///< Countdown to phase end         — label "Remaining"
    TIME_ELAPSED,        ///< Count-up from phase start      — label "Elapsed"
    DISTANCE             ///< Distance remaining to phase end — label "km/mi remaining"
};

/**
 * @brief Snapshot of interval training state; updated every tick by the Service.
 *
 * All distance values are in metres; the GUI converts to km/mi as needed.
 * phaseTimerSec is remaining / elapsed / open time depending on metric.
 */
struct IntervalsData {
    IntervalsPhase  phase         = IntervalsPhase::WARM_UP;
    IntervalsMetric metric        = IntervalsMetric::TIME_OPEN;
    uint8_t         repeat        = 0;    ///< 1-based current repeat index
    uint8_t         totalRepeats  = 0;    ///< Total number of RUN-REST cycles
    std::time_t     phaseTimerSec = 0;    ///< Seconds (meaning depends on metric)
    float           distRemaining = 0.0f; ///< Metres remaining (DISTANCE metric only)
    bool            runWalk       = false; ///< Run/Walk: the REST phase is shown as "Walk"
};

// =============================================================================
// Pace and heart-rate range alerts
// =============================================================================

/**
 * @brief Which range alert played; see CustomMessage::RunAlert.
 */
enum class RunAlertKind : uint8_t {
    PACE_FAST = 0,  ///< Quicker than the Too Fast pace
    PACE_SLOW,      ///< Slower than the Too Slow pace
    HR_HIGH,        ///< Above HR High
    HR_LOW          ///< Below HR Low
};

// =============================================================================

/**
 * @brief Real-time metrics snapshot; updated every second by the Service.
 *
 * All distances are in metres, speeds in m/s, pace in s/m, times in seconds.
 * The GUI converts to display units as needed.
 *
 * @note This snapshot exists for the GUI only. The live speed and pace it
 *       carries are corrected against the position track and smoothed. The
 *       averages are not: they come from the distance and time totals.
 */
/**
 * Where the run is on a track, each tick (Run::TrackSource::status()).
 */
struct OnTrack {
    enum Segment : uint8_t { NONE = 0, ROAD, TRACK };   ///< As Oval::Session::Segment
    /// As Run::CalState
    enum Cal : uint8_t { CAL_NONE = 0, CALIBRATING, PROVISIONAL, LOCKED, NOT_STANDARD, STORED };

    uint8_t  segment        = NONE;
    uint8_t  trackNumber    = 0;     ///< A stored track's number; 0 if not stored
    uint8_t  lane           = 0;     ///< 0: "No lane" (the road)
    uint8_t  cal            = CAL_NONE;
    uint8_t  calLaps        = 0;     ///< Laps of calibration so far (until locked)
    uint8_t  newTrackNumber = 0;     ///< The number a new track would be saved as; 0 if none free
    bool     weakGps        = false;
    uint16_t autoJoins      = 0;     ///< Known tracks the run joined by itself, this run (the notice)
    float    trackM         = 0.0f;  ///< Distance on a track so far
};

struct Data {

    // Pace, s/m
    float pace        = 0.0f;   ///< Live pace, corrected and smoothed (see GpsSpeedFilter)
    float avgPace     = 0.0f;
    float lapPace     = 0.0f;

    // Distance, m
    float distance    = 0.0f;
    float lapDistance = 0.0f;

    // Time, s
    std::time_t totalTime = 0;
    std::time_t lapTime   = 0;

    uint32_t lapNum = 0;

    // Heart rate, bpm
    float hr            = 0.0f;
    float hrTrustLevel  = 0.0f;
    uint8_t hrSource    = 0;     // SDK HeartRate::Source: 0 none/unknown, 1 optical, 2 external
    float avgHR         = 0.0f;
    float maxHR         = 0.0f;
    float avgLapHR      = 0.0f;
    float maxLapHR      = 0.0f;

    // Speed, m/s
    float speed        = 0.0f;   ///< Live speed, smoothed over the same window as pace
    float avgSpeed     = 0.0f;
    float maxSpeed     = 0.0f;
    float avgLapSpeed  = 0.0f;
    float maxLapSpeed  = 0.0f;

    // Elevation, m (current altitude from barometer)
    float elevation    = 0.0f;

    bool intervalsMode = false; ///< true when the track was started in intervals mode
    bool workoutMode   = false; ///< true while a structured workout runs; WORKOUT_DATA carries its face

    // Interval training (populated only when intervalsMode == true)
    IntervalsData intervals {};

    OnTrack onTrack {};
};

} // namespace Track

#endif // TRACK_HPP
