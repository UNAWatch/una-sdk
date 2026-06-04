/**
 ******************************************************************************
 * @file    OutdoorStrideCalibConfig.hpp
 * @brief   Tunable constants and bin layout for the outdoor stride calibrator.
 *
 * These values are not user-configurable but are firmware-tunable for
 * field adjustment.
 ******************************************************************************
 */

#ifndef __OUTDOOR_STRIDE_CALIB_CONFIG_HPP
#define __OUTDOOR_STRIDE_CALIB_CONFIG_HPP

#include <cstddef>
#include <cstdint>

namespace SDK::Calibration
{

/**
 * @brief Outdoor stride calibrator tunables and bin layout.
 */
namespace Config
{

// --- Acceptance state machine ---------------------------------

/// Consecutive qualifying seconds before samples are accepted.
constexpr float kSteadyStateMinSeconds = 15.0f;

/// Max delta_t between ticks before a stream gap resets the counter.
constexpr float kMaxTickGapS = 1.5f;

/// Fractional +/- band for speed and cadence steady-state (5%).
constexpr float kSteadyBandFrac = 0.05f;

/// GPS speed acceptance window, m/s.
constexpr float kGpsSpeedMinMs = 0.5f;
constexpr float kGpsSpeedMaxMs = 8.0f;

/// Cadence acceptance window, SPM (matches the LUT range).
constexpr float kCadenceMinSpm = 80.0f;
constexpr float kCadenceMaxSpm = 220.0f;

/// Maximum absolute terrain grade for acceptance, percent.
constexpr float kGradeMaxPct = 3.0f;

/// Rolling window for barometric grade computation, seconds (kernel).
constexpr float kGradeWindowS = 10.0f;

/// Implied stride-length plausibility bounds (post-gate sanity check), metres.
constexpr float kStrideMinM = 0.3f;
constexpr float kStrideMaxM = 5.0f;

// --- Bin aggregation ----------------------------------------------

/// Per-bin distance cap before aging kicks in, metres.
constexpr float kDistanceCapM = 20000.0f;

/// Minimum accumulated steps for a bin to be considered valid (~100 strides).
constexpr float kBinValidMinSteps = 200.0f;

// --- Phase-2 gate -------------------------------------------------

/// Valid bins required for the phase-2 gate.
constexpr size_t kOutdoorLutMinValidBins = 8;

/// Total LUT distance required for the phase-2 gate, metres.
constexpr float kOutdoorLutMinCalibrationDistanceM = 5000.0f;

// --- Intermediate (outdoor-estimate) gate -------------------------

/// Valid bins required to START using the outdoor LUT for live estimation
/// while the delta LUT stays frozen (the delta only begins learning at the
/// full phase-2 gate above). A single valid bin already yields a personalised
/// constant stride, which is no worse than the cadence-independent demographic
/// default -- so one valid bin is enough to switch over. There is deliberately
/// no distance floor (a valid bin already implies one bin's worth of steps).
constexpr size_t kOutdoorLutMinValidBinsEstimate = 1;

// --- Bin layout -------------------------------------------------

/// Cadence bin width, SPM.
constexpr float kBinWidthSpm = 4.0f;

/// Lowest bin lower-edge cadence, SPM (bin 0 covers [80, 84)).
constexpr float kBinBaseSpm = 80.0f;

/// Number of cadence bins: centres 82, 86, ..., 218.
constexpr size_t kBinCount = 35;

} // namespace Config

} // namespace SDK::Calibration

#endif /* __OUTDOOR_STRIDE_CALIB_CONFIG_HPP */
