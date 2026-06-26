/**
 ******************************************************************************
 * @file    FitRecordCadence.cpp
 * @brief   Pure helpers for FIT record cadence / step_length encoding.
 ******************************************************************************
 */

#include "SDK/Fit/FitRecordCadence.hpp"

#include <algorithm>
#include <cmath>

namespace SDK::FitRecordCadence
{

CadenceFitFields encodeCadenceSpm(float cadenceSpm)
{
    CadenceFitFields out{};
    if (!std::isfinite(cadenceSpm) || cadenceSpm < 0.0f) {
        cadenceSpm = 0.0f;
    }

    // Internal cadence is steps/min; FIT record.cadence uses strides/min (Garmin running).
    const float fitCadenceSpm = cadenceSpm * 0.5f;

    const float capped = std::max(0.0f, std::min(255.0f, fitCadenceSpm));
    const float whole  = std::floor(capped);
    const float frac   = capped - whole;

    const long fractional =
        std::min(127L, std::lround(frac * 128.0f));

    out.cadence = static_cast<uint8_t>(whole);
    out.fractionalCadence = static_cast<uint8_t>(fractional);

    return out;
}

uint16_t encodeStepLengthM(float stepLengthM)
{
    if (!std::isfinite(stepLengthM) || stepLengthM < 0.0f) {
        return 0;
    }
    const float mmScaled = stepLengthM * 1000.0f * 10.0f;
    const long rounded = std::max(0L, std::min(65535L, std::lround(mmScaled)));
    return static_cast<uint16_t>(rounded);
}

} // namespace SDK::FitRecordCadence
