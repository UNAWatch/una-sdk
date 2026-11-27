/*******************************************************************************
 * @file   Sample.hpp
 * @brief  Oval: one GPS reading as the service passes it in, plus wrap-safe
 *         helpers for the sensor's millisecond clock.
 *
 *         The Oval code is plain C++17 with no SDK dependency, so the same
 *         sources build for the watch and for the host tests.
 ******************************************************************************/

#ifndef OVAL_SAMPLE_HPP
#define OVAL_SAMPLE_HPP

#include <cstdint>
#include <limits>

namespace Oval {

/** One GPS_LOCATION reading, with the kernel's running GPS distance. */
struct Sample {
    /** Sensor timestamp in ms (SensorDataParserGpsLocation::getTimestamp()).
     *  Wraps after about 49.7 days; all Oval arithmetic on it is wrap-safe. */
    uint32_t tMs = 0;

    /** Degrees. The sensor gives float; double here so the subtraction of
     *  the origin in LocalFrame keeps sub-metre precision. */
    double lat = 0.0;
    double lon = 0.0;

    /** HDOP: the parser's precision figure (getPrecision()), read as the
     *  horizontal dilution of precision, a unitless measure of the satellite
     *  geometry: about 1 is good, 2 or more is poor. Negative means
     *  unknown. hdopErrorM() in Config.hpp turns it into an estimated error
     *  in metres. */
    float hdop = -1.0f;

    /** Coordinates valid (isCoordinatesValid()). */
    bool fix = false;

    /** The kernel's cumulative GPS_DISTANCE at this moment, in metres, or
     *  NaN when it is not available. Only differences are used. */
    float gpsDistM = std::numeric_limits<float>::quiet_NaN();

    /** GPS altitude in metres, or NaN when not available. Only calibration
     *  uses it, for the track's constant elevation. */
    float altM = std::numeric_limits<float>::quiet_NaN();
};

/** Signed difference later - earlier on the wrapping ms clock. Correct while
 *  the two times are less than about 24.8 days apart. */
inline int32_t msDiff(uint32_t later, uint32_t earlier) noexcept
{
    return static_cast<int32_t>(later - earlier);
}

/** True if a is strictly later than b on the wrapping ms clock. */
inline bool msAfter(uint32_t a, uint32_t b) noexcept
{
    return msDiff(a, b) > 0;
}

} // namespace Oval

#endif // OVAL_SAMPLE_HPP
