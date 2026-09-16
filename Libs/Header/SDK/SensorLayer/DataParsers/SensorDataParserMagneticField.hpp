/**
 ******************************************************************************
 * @file    SensorDataParserMagneticField.hpp
 * @date    04-September-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Parser for MAGNETIC_FIELD sensor samples (3-axis magnetic field).
 *
 * The field is reported in microtesla, in the watch's own axes, with whatever
 * hard- and soft-iron correction is in force applied.
 *
 * MAG_CALIBRATED says whether there was one. A watch that has never been
 * calibrated still produces samples: they carry the part's own offsets, which
 * is what a calibration run needs and is not something to take a direction
 * from.
 *
 * A compass bearing is a function of one sample and nothing else, so it is
 * computed here rather than published as a stream of its own: there is one
 * measurement, and a heading is a way of reading it. getAzimuthDeg() is
 * meaningful only when isAzimuthValid().
 *
 * Both bearings are the bearing of 12 o'clock. On the wrist 12 o'clock runs
 * along the forearm, so pointing the arm at something points 12 o'clock at it.
 *
 * getAzimuthDeg() is not tilt-compensated: it is worked out in the watch's own
 * XY plane, so it is a heading only while the watch is held roughly level.
 * Tilting it rotates part of the vertical field into that plane, and where the
 * field is steep - at 66 degrees of inclination the vertical part is more than
 * twice the horizontal - a quarter of the way up is enough to turn the answer
 * round.
 *
 * Compensating for that needs gravity, which no magnetic sample carries, so it
 * is passed in: read the accelerometer wherever you already read it and call
 * getAzimuthDegTilted(). Nothing here subscribes to anything on your behalf,
 * and the two bearings are the same arithmetic - level projection first, then
 * one common last step - so a watch held flat reads the same either way.
 * Raising the arm and turning the wrist about it, together or apart, leave the
 * tilted bearing where it was. It has no answer only while 12 o'clock points
 * nearly straight up or down, where the forearm has no direction on the map.
 *
 * Neither bearing is true north. No declination is applied, so a bearing taken
 * against a map needs the local declination added by whoever knows where the
 * watch is.
 ******************************************************************************
 */

#ifndef SENSOR_DATA_PARSER_MAGNETIC_FIELD_HPP
#define SENSOR_DATA_PARSER_MAGNETIC_FIELD_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cmath>
#include <cstdint>

namespace SDK::SensorDataParser
{

class MagneticField
{
public:
    enum Field : uint8_t {
        MAG_X = 0,  ///< Magnetic field along X, microtesla.
        MAG_Y = 1,  ///< Magnetic field along Y, microtesla.
        MAG_Z = 2,  ///< Magnetic field along Z, microtesla.
        MAG_CALIBRATED = 3,  ///< 1 when a correction was applied to this sample.
        COUNT
    };

    explicit MagneticField(const SDK::Sensor::DataView data) : mData(data) {}

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

    bool isCalibrated() const
    {
        return isDataValid() && (mData.u[Field::MAG_CALIBRATED] != 0);
    }

    /**
     * @brief   Whether this sample supports a bearing at all.
     * @note    False without a calibration, and false when the horizontal
     *          field is too weak to have a direction - the watch edge-on to
     *          the field, or against something magnetic that has cancelled it.
     */
    bool isAzimuthValid() const
    {
        return isCalibrated() && hasDirection(getX(), getY());
    }

    /**
     * @brief   Bearing clockwise from magnetic north, degrees in [0, 360).
     * @note    Level only. Zero when there is no bearing to give, which is why
     *          it is read together with isAzimuthValid() and not on its own.
     */
    float getAzimuthDeg() const
    {
        return isAzimuthValid() ? bearingDeg(getX(), getY()) : 0.0f;
    }

    /**
     * @brief   Bearing of 12 o'clock clockwise from magnetic north, corrected
     *          for how the watch is being held.
     * @param   ax, ay, az: The accelerometer's own reading, in the watch's
     *          axes - not a vector pointing down. A watch lying face-up at
     *          rest reads az positive. Passed the other way round the bearing
     *          comes out mirrored, and nothing here can catch that, because
     *          face-down is a real attitude. Any unit: only the direction is
     *          used. Take it from a sample close in time to this one - a
     *          bearing compensated with gravity from a second ago is a bearing
     *          for how the watch was held a second ago.
     * @param   degrees: The bearing, untouched unless this returns true.
     * @retval  'false' when there is no trustworthy bearing: no calibration,
     *          too little horizontal field, gravity that cannot say which way
     *          is down, or 12 o'clock pointing too near straight up or down to
     *          have a direction on the map.
     */
    bool getAzimuthDegTilted(float ax, float ay, float az,
                             float& degrees) const
    {
        float xh = 0.0f;
        float yh = 0.0f;

        if (!isCalibrated()) {
            return false;
        }

        if (!levelProject(getX(), getY(), getZ(), ax, ay, az, xh, yh)) {
            return false;
        }

        if (!hasDirection(xh, yh)) {
            return false;
        }

        degrees = bearingDeg(xh, yh);

        return true;
    }

    /// Below this the horizontal field is too small to give a direction. Well
    /// under any real horizontal field, which even at high latitude does not
    /// come near it.
    static constexpr float MIN_HORIZONTAL_UT = 3.0f;

    /**
     * @brief   Whether a field has a horizontal component worth a bearing.
     * @note    Static and free of the sample, and part of the API: it serves a
     *          field that did not arrive in a MagneticField sample, such as one
     *          from a caller's own fusion.
     */
    static bool hasDirection(float x, float y)
    {
        // Compared as squares to avoid a square root. A NaN fails every
        // comparison, so this answers false for one rather than letting it
        // through to atan2.
        return ((x * x) + (y * y)) >= (MIN_HORIZONTAL_UT * MIN_HORIZONTAL_UT);
    }

    /**
     * @brief   Project a field into the horizontal plane using gravity.
     * @note    This is the whole of tilt compensation. It lays 12 o'clock flat -
     *          the watch's +Y less its part along gravity - and returns the
     *          field along that direction as yh, and along the horizontal
     *          direction a quarter turn clockwise from it (3 o'clock laid flat)
     *          as xh. What comes out is read by the same bearingDeg() as a
     *          level sample, and gives the bearing of 12 o'clock. Held flat it
     *          is the identity - (xh, yh) come back as (x, y) - which is why
     *          the two bearings agree there.
     * @note    No angles are taken out of gravity on the way. Splitting a tilt
     *          into two angles and undoing them one at a time reads 12 o'clock
     *          only while one of the two is zero; with the arm raised and the
     *          wrist turned at once it is tens of degrees out.
     * @note    Static, and part of the API for the same reason as
     *          hasDirection().
     * @param   x, y, z:    Corrected field, microtesla.
     * @param   ax, ay, az: The accelerometer's reading in the same axes, as
     *          getAzimuthDegTilted() takes it: az positive face-up. Any unit.
     * @param   xh, yh:     Horizontal field, microtesla, along 3 o'clock and 12
     *                      o'clock laid flat; untouched unless this returns
     *                      true.
     * @retval  'false' when gravity cannot say which way is down, or when
     *          12 o'clock points so near straight up or down that there is too
     *          little of it left once laid flat to give a direction.
     */
    static bool levelProject(float x, float y, float z,
                             float ax, float ay, float az,
                             float& xh, float& yh)
    {
        const float magnitude = std::sqrt((ax * ax) + (ay * ay) + (az * az));

        const float nx = ax / magnitude;
        const float ny = ay / magnitude;
        const float nz = az / magnitude;

        // What is left of 12 o'clock once laid flat, squared: with gravity a
        // unit vector, 1 - ny^2 = nx^2 + nz^2. Gravity of zero length or with
        // a NaN in it leaves this a NaN, which fails the test below, so it is
        // rejected here rather than spreading through the arithmetic.
        const float flatSq = (nx * nx) + (nz * nz);

        // 12 o'clock pointing at the sky or the ground: the forearm has no
        // direction on the map, and near it noise is divided by a length close
        // to zero. Reported as no bearing rather than as a number that spins.
        if (!(flatSq >= MIN_FLAT_SQ)) {
            return false;
        }

        const float flat = std::sqrt(flatSq);

        // The field's part along gravity, taken out of both components. Each
        // direction is at right angles to gravity, so nothing vertical is
        // left in either.
        const float along = (x * nx) + (y * ny) + (z * nz);

        xh = ((x * nz) - (z * nx)) / flat;
        yh = (y - (ny * along)) / flat;

        return true;
    }

    /// How much of 12 o'clock must be left once laid flat for a projection to
    /// mean anything, as the square of its share. 0.03 is 12 o'clock about
    /// 10 degrees from pointing straight up or down.
    static constexpr float MIN_FLAT_SQ = 0.03f;

    /**
     * @brief   Bearing clockwise from magnetic north, degrees in [0, 360).
     * @note    The axis convention is the watch's own: +X out of the 3 o'clock
     *          side, +Y out of the 12 o'clock side. The bearing is that of the
     *          12 o'clock direction, which is what someone pointing the watch
     *          at something expects to read.
     * @note    Takes a horizontal field: either a level sample's own x and y,
     *          or what levelProject() returns for one that is not level.
     */
    static float bearingDeg(float x, float y)
    {
        constexpr float RAD_TO_DEG = 57.29577951308232f;

        // atan2(-x, y) rather than the textbook atan2(y, x): a bearing runs
        // clockwise from +Y where the mathematical angle runs anticlockwise
        // from +X.
        float degrees = std::atan2(-x, y) * RAD_TO_DEG;

        if (degrees < 0.0f) {
            degrees += 360.0f;
        }

        // Guards the two ways the wrap can land outside the promised range: a
        // value that rounds up onto 360, and a NaN, which fails this test and
        // is replaced rather than returned.
        if (!(degrees < 360.0f)) {
            degrees = 0.0f;
        }

        return degrees;
    }

    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0;
    }

    /**
     * @brief   When the field was measured, in microseconds.
     * @note    The instant of the measurement, not of its delivery, so a log
     *          that writes this beside an accelerometer sample lines the two
     *          up on one time base.
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

#endif // SENSOR_DATA_PARSER_MAGNETIC_FIELD_HPP
