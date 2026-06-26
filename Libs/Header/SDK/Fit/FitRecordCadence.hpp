/**
 ******************************************************************************
 * @file    FitRecordCadence.hpp
 * @brief   Pure helpers for FIT record cadence / step_length encoding.
 ******************************************************************************
 */

#ifndef FIT_RECORD_CADENCE_HPP
#define FIT_RECORD_CADENCE_HPP

#include <cstdint>

namespace SDK::FitRecordCadence
{

struct CadenceFitFields {
    uint8_t cadence            = 0;
    uint8_t fractionalCadence  = 0;
};

/** Encode internal steps/min into FIT strides/min (÷2) as cadence + fractional_cadence. */
CadenceFitFields encodeCadenceSpm(float cadenceSpm);

/** Encode step length in meters to FIT record.step_length raw value (0.1 mm units). */
uint16_t encodeStepLengthM(float stepLengthM);

} // namespace SDK::FitRecordCadence

#endif // FIT_RECORD_CADENCE_HPP
