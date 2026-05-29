/**
 ******************************************************************************
 * @file    StrideMath.hpp
 * @brief   Stateless SDK-side stride / step-length helpers.
 *
 * Holds the implied step-length derivation that previously lived in the kernel
 * (CadenceEstimator::updateStepLength) — see Outdoor-Data-Collection.md §3.6 and
 * Treadmill-Specification.md §8.1. Kept header-only and stateless so it is
 * shared by the FIT record writer (step_length output) and the calibrator's
 * stride-plausibility gate (§4.3) without duplicating the formula.
 ******************************************************************************
 */

#ifndef __STRIDE_MATH_HPP
#define __STRIDE_MATH_HPP

#include <cmath>

namespace SDK::Calibration
{

/**
 * @brief Stateless stride / step-length math.
 *
 * Definitions (matching the spec):
 *   - step length  = distance per single foot strike  = v * 60 / cadence_spm
 *   - stride length = distance left-to-left (2 steps)  = v * 120 / cadence_spm
 *
 * where v is GPS horizontal speed (m/s) and cadence_spm is steps per minute.
 */
namespace StrideMath
{

/// FIT step-length emit gate, metres. Previously kMin/kMaxStepLengthM in the
/// kernel CadenceEstimatorConfig; moved here as writer policy (spec §3.6.4).
constexpr float kMinStepLengthM = 0.15f;
constexpr float kMaxStepLengthM = 2.50f;

/// Result of an implied step-length computation.
struct StepLength {
    float meters;  ///< Computed step length, m (defined only when valid).
    bool  valid;   ///< true when both inputs were valid and the result is in-gate.
};

/**
 * @brief Implied step length from speed and cadence, with the FIT emit gate.
 *
 * Replaces the removed kernel CadenceEstimator::updateStepLength(): the Running
 * app calls this at record-write time and gates the result before writing
 * record.step_length, preserving identical output values (spec §3.6.5).
 *
 * @param speedMps     GPS horizontal speed, m/s.
 * @param speedValid   true when speedMps is a reliable current estimate.
 * @param cadenceSpm   Cadence, steps per minute.
 * @param cadenceValid true when cadenceSpm is a reliable current estimate.
 * @return {meters, valid}. valid is false if either input is invalid, cadence
 *         is non-positive, or the result falls outside [kMin, kMax]StepLengthM.
 */
inline StepLength impliedStepLengthM(float speedMps, bool speedValid,
                                     float cadenceSpm, bool cadenceValid)
{
    if (!speedValid || !cadenceValid || cadenceSpm <= 0.0f) {
        return {0.0f, false};
    }

    const float stepM = (speedMps * 60.0f) / cadenceSpm;
    // Reject non-finite results (e.g. NaN/inf speed): NaN range comparisons are
    // all false, which would otherwise mark a NaN step length valid and leak it
    // into record.step_length.
    if (!std::isfinite(stepM) ||
        stepM < kMinStepLengthM || stepM > kMaxStepLengthM) {
        return {stepM, false};
    }
    return {stepM, true};
}

/**
 * @brief Implied stride length (left-to-left) from speed and cadence.
 *
 * Used by the calibrator's post-gate plausibility check (spec §4.3); the caller
 * supplies the bounds. No internal gating here — this is the raw geometric
 * value SL_implied = v * 120 / cadence.
 *
 * @param speedMps   GPS horizontal speed, m/s.
 * @param cadenceSpm Cadence, steps per minute (assumed > 0 by the caller).
 * @return Stride length in metres.
 */
inline float impliedStrideLengthM(float speedMps, float cadenceSpm)
{
    if (cadenceSpm <= 0.0f) {
        return 0.0f;  // guard against div-by-zero / inf-NaN for invalid cadence
    }
    return (speedMps * 120.0f) / cadenceSpm;
}

} // namespace StrideMath

} // namespace SDK::Calibration

#endif /* __STRIDE_MATH_HPP */
