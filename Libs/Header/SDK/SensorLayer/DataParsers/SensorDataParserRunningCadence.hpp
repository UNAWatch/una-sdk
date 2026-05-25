/**
 ******************************************************************************
 * @file    SensorDataParserRunningCadence.hpp
 * @brief   Parser for RUNNING_CADENCE sensor samples.
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_RUNNING_CADENCE_HPP
#define __SENSOR_DATA_PARSER_RUNNING_CADENCE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{

class RunningCadence
{
public:
    enum Field : uint8_t {
        CADENCE_SPM        = 0,
        CADENCE_VALID      = 1,
        STEP_LENGTH_M      = 2,
        STEP_LENGTH_VALID  = 3,
        COUNT
    };

    explicit RunningCadence(const SDK::Sensor::DataView data) : mData(data) {}

    bool isDataValid() const
    {
        return mData.getFieldCount() == Field::COUNT;
    }

    float getCadenceSpm() const
    {
        return isDataValid() ? mData.f[Field::CADENCE_SPM] : 0.0f;
    }

    bool isCadenceValid() const
    {
        return isDataValid() && (mData.u[Field::CADENCE_VALID] != 0);
    }

    float getStepLengthM() const
    {
        return isDataValid() ? mData.f[Field::STEP_LENGTH_M] : 0.0f;
    }

    bool isStepLengthValid() const
    {
        return isDataValid() && (mData.u[Field::STEP_LENGTH_VALID] != 0);
    }

    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0;
    }

    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
};

} // namespace SDK::SensorDataParser

#endif /* __SENSOR_DATA_PARSER_RUNNING_CADENCE_HPP */
