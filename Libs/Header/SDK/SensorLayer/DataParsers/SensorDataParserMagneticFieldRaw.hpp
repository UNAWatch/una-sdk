/**
 ******************************************************************************
 * @file    SensorDataParserMagneticFieldRaw.hpp
 * @date    04-September-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Parser for MAGNETIC_FIELD_RAW sensor samples.
 *
 * The field exactly as the part measured it, in microtesla, with no correction
 * applied - offsets, scale and cross-axis errors all still in it.
 *
 * This is what a calibration is worked out from, and what tells you how big
 * the part's own errors are. It is not what a direction is taken from: there
 * is deliberately no bearing here, because a bearing from an uncorrected field
 * is wrong by however far the offsets happen to push it. Use MAGNETIC_FIELD
 * for that, which carries the correction and says whether there was one.
 ******************************************************************************
 */

#ifndef SENSOR_DATA_PARSER_MAGNETIC_FIELD_RAW_HPP
#define SENSOR_DATA_PARSER_MAGNETIC_FIELD_RAW_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{

class MagneticFieldRaw
{
public:
    enum Field : uint8_t {
        MAG_X = 0,  ///< Measured field along X, microtesla.
        MAG_Y = 1,  ///< Measured field along Y, microtesla.
        MAG_Z = 2,  ///< Measured field along Z, microtesla.
        COUNT
    };

    explicit MagneticFieldRaw(const SDK::Sensor::DataView data) : mData(data) {}

    bool isDataValid() const
    {
        return mData.getFieldCount() == Field::COUNT;
    }

    float getX() const
    {
        return isDataValid() ? mData.f[Field::MAG_X] : 0.0f;
    }

    float getY() const
    {
        return isDataValid() ? mData.f[Field::MAG_Y] : 0.0f;
    }

    float getZ() const
    {
        return isDataValid() ? mData.f[Field::MAG_Z] : 0.0f;
    }

    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0;
    }

    /**
     * @brief   When the field was measured, in microseconds.
     * @retval  Microseconds, or 0 if the sample is not valid.
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData.getTimestampUs() : 0;
    }

    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
};

} // namespace SDK::SensorDataParser

#endif // SENSOR_DATA_PARSER_MAGNETIC_FIELD_RAW_HPP
