/**
 ******************************************************************************
 * @file    SensorDataParserPressure.hpp
 * @date    27-November-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Pressure sensor
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_PRESSURE_HPP
#define __SENSOR_DATA_PARSER_PRESSURE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>
#include <limits>
#include <cmath>

namespace SDK {
namespace SensorDataParser {

/**
 * @class   Pressure
 * @brief   SensorData parser for the Pressure sensor.
 *
 * @details
 * This parser provides type-safe accessors to the underlying sensor data fields.
 * It does not own the underlying storage (zero-copy view).
 *
 * @note
 * Units:
 *  - Field::PRESS and Field::PRESS_SEA_LEVEL are stored in Pa (Pascal).
 */
class Pressure
{
public:
    /**
     * @enum    Field
     * @brief   Indices of fields inside the Sensor::DataView.
     */
    enum Field : uint8_t {
        PRESS = 0,          ///< Station pressure, Pa
        PRESS_SEA_LEVEL,    ///< QNH / sea-level pressure, Pa
        COUNT               ///< Number of fields (must be last)
    };

    /**
     * @brief   Construct parser from a reference to sensor data.
     * @param   data Reference to sensor data view (does not transfer ownership).
     */
    explicit Pressure(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief   Check if data layout matches this parser.
     *
     * @details
     * This verifies that the DataView has the expected number of float fields.
     * It does not validate numerical ranges (e.g. pressure > 0).
     *
     * @return  True if the field count matches Field::COUNT.
     */
    bool isDataValid() const
    {
        return (mData.getFieldCount() == Field::COUNT);
    }

    /**
     * @brief   Get station pressure.
     * @return  Pressure in Pa. Returns -1.0f if invalid.
     */
    float getPressure() const
    {
        return isDataValid() ? mData.f[Field::PRESS] : -1.0f;
    }

    /**
     * @brief   Get sea-level pressure (QNH).
     *
     * @details
     * QNH is the pressure reduced to mean sea level, used as a reference for
     * barometric altitude calculation.
     *
     * @return  Pressure in Pa. Returns -1.0f if invalid.
     */
    float getP0() const
    {
        return isDataValid() ? mData.f[Field::PRESS_SEA_LEVEL] : -1.0f;
    }

    /**
     * @brief   Get timestamp of the data.
     * @return  Data timestamp in ms (0 if invalid).
     */
    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0U;
    }

    /**
     * @brief   Get data timestamp in microseconds.
     * @return  Data timestamp in µs (0 if invalid).
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData.getTimestampUs() : 0ULL;
    }

    /**
     * @brief   Returns the number of fields expected by this parser.
     * @return  The number of fields (Field::COUNT).
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::COUNT);
    }

    /**
     * @brief   Compute altitude using ISA barometric formula (no temperature correction).
     *
     * @details
     * Uses station pressure @ref Field::PRESS and sea-level pressure (QNH)
     * @ref Field::PRESS_SEA_LEVEL.
     *
     * The formula used:
     * \f[
     *   h = 44330 \cdot \left(1 - \left(\frac{p}{p_0}\right)^{\frac{1}{5.255877}}\right)
     * \f]
     *
     * @note
     * Both @p p and @p p0 must be > 0, otherwise NaN is returned.
     * If you want a fallback to ISA standard (101325 Pa) when p0 is missing,
     * implement it at the producer side or adjust this function accordingly.
     *
     * @return  Altitude in meters. Returns NaN if data or inputs are invalid.
     */
    float getAltitude() const
    {
        if (!isDataValid()) {
            return std::numeric_limits<float>::quiet_NaN();
        }

        const float p  = mData.f[Field::PRESS];
        const float p0 = mData.f[Field::PRESS_SEA_LEVEL];

        return getAltitude(p, p0);
    }

    /**
     * @brief   Altitude from station pressure and sea-level pressure (QNH).
     *
     * @param   p   Station pressure (Pa).
     * @param   p0  Sea-level pressure / QNH (Pa).
     *
     * @return  Altitude in meters. Returns NaN if inputs are invalid.
     */
    static float getAltitude(float p, float p0)
    {
        if (!(p > 0.0f) || !(p0 > 0.0f)) {
            return std::numeric_limits<float>::quiet_NaN();
        }

        /// Same constants as altitudeISA(), generalized to p0.
        static constexpr float inv_n = 1.0f / 5.255877f;

        return 44330.0f * (1.0f - std::pow(p / p0, inv_n));
    }

private:
    /// Underlying sensor data view (non-owning).
    const SDK::Sensor::DataView mData;
}; /* class Pressure */

} /* namespace SensorDataParser */
} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_PRESSURE_HPP */
