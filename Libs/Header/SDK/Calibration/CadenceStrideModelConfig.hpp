/**
 ******************************************************************************
 * @file    CadenceStrideModelConfig.hpp
 * @brief   Tunables for the read-side cadence/stride model and live estimator.
 *
 * Extends the existing SDK::Calibration::Config namespace. Reuses the outdoor
 * calibrator's bin layout, stride bounds, phase-2 gate, and tick-gap clamp from
 * OutdoorStrideCalibConfig.hpp — those are NOT duplicated here.
 ******************************************************************************
 */

#ifndef __CADENCE_STRIDE_MODEL_CONFIG_HPP
#define __CADENCE_STRIDE_MODEL_CONFIG_HPP

#include <cstdint>

namespace SDK::Calibration
{

namespace Config
{

// --- Phase-1 demographic stride ----------------------------------------------

/// Unisex demographic stride multiplier: SL_demographic = kDemographicK * h.
/// Gender refinement is deliberately deferred; only this constant changes when
/// gender is later added.
constexpr float kDemographicK = 0.685f;

/// Fallback height (m) when the profile height is missing/implausible.
constexpr float kDefaultHeightM = 1.70f;

/// Plausible height clamp window, metres.
constexpr float kHeightMinM = 1.40f;
constexpr float kHeightMaxM = 2.10f;

// --- Post-run delta learning -------------------------------------------------

/// Post-run delta learning rate η (0 < η <= 1).
constexpr float kLearningRateEta = 0.4f;

/// D_actual sanity band as a fraction of D_estimated.
constexpr float kDActualRatioMin = 0.5f;
constexpr float kDActualRatioMax = 2.0f;

// --- Live estimator ----------------------------------------------------------

/// Live hold-forward window on cadence loss, seconds.
constexpr float kCadenceHoldSeconds = 5.0f;

// --- Delta LUT persistence ---------------------------------------------------

/// Delta-LUT filename in the Treadmill app's own root (NOT under SharedData).
constexpr const char *kDeltaLutPath = "treadmill_delta.json";

/// Delta-LUT schema version.
constexpr int32_t kDeltaLutVersion = 1;

} // namespace Config

} // namespace SDK::Calibration

#endif /* __CADENCE_STRIDE_MODEL_CONFIG_HPP */
