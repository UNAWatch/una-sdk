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
 * getAzimuthDeg() is not tilt-compensated: it is worked out in the watch's own
 * XY plane, so it is a heading only while the watch is held roughly level.
 * Tilting it rotates part of the horizontal field out of that plane and swings
 * the answer.
 *
 * Compensating for that needs gravity, which no magnetic sample carries, so it
 * is passed in: read the accelerometer wherever you already read it and call
 * getAzimuthDegTilted(). Nothing here subscribes to anything on your behalf,
 * and the two bearings are the same arithmetic - level projection first, then
 * one common last step - so a watch held flat reads the same either way.
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
     * @brief   Bearing clockwise from magnetic north, corrected for how the
     *          watch is being held.
     * @param   ax, ay, az: Gravity in the watch's axes, as the accelerometer
     *          reports it. Any unit: only the direction is used. Take it from
     *          a sample close in time to this one - a bearing compensated with
     *          gravity from a second ago is a bearing for how the watch was
     *          held a second ago.
     * @param   degrees: The bearing, untouched unless this returns true.
     * @retval  'false' when there is no trustworthy bearing: no calibration,
     *          too little horizontal field, or gravity that cannot say which
     *          way is down.
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
     * @note    Static and free of the sample, so the arithmetic can be
     *          exercised directly rather than through a constructed view.
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
     * @note    This is the whole of tilt compensation: it rotates the measured
     *          field into the frame the watch would be in if it were level, so
     *          that what comes out can be read by the same bearingDeg() as a
     *          level sample. Held flat it is the identity - (xh, yh) come back
     *          as (x, y) - which is why the two bearings agree there.
     * @param   x, y, z:    Corrected field, microtesla.
     * @param   ax, ay, az: Gravity in the same axes. Any unit.
     * @param   xh, yh:     Level-frame horizontal field, untouched unless this
     *                      returns true.
     * @retval  'false' when gravity cannot say which way is down: too small to
     *          be gravity at all, or leaving the watch so near edge-on that
     *          the projection is undefined.
     */
    static bool levelProject(float x, float y, float z,
                             float ax, float ay, float az,
                             float& xh, float& yh)
    {
        const float magnitude = std::sqrt((ax * ax) + (ay * ay) + (az * az));

        // A NaN fails this, so one is rejected here rather than spreading
        // through the trigonometry below.
        if (!(magnitude > 0.0f)) {
            return false;
        }

        const float nx = ax / magnitude;
        const float ny = ay / magnitude;
        const float nz = az / magnitude;

        // Pitch about Y and roll about X, taken straight from gravity: with
        // the watch level, gravity lies along Z and both are zero.
        const float sinPitch = -nx;
        const float cosPitchSq = 1.0f - (sinPitch * sinPitch);

        // Edge-on: X is pointing at the ground, the roll axis is along
        // gravity, and how far the watch is rolled about it is not something
        // gravity can answer. Reported as no bearing rather than as a number
        // that spins.
        if (cosPitchSq < MIN_COS_PITCH_SQ) {
            return false;
        }

        const float cosPitch = std::sqrt(cosPitchSq);
        const float sinRoll  = ny / cosPitch;
        const float cosRoll  = nz / cosPitch;

        // The rotation the angles above describe, applied in the order they
        // were taken out of gravity. Getting that order wrong leaves the two
        // consistent one axis at a time and wrong together, which is why the
        // test for this feeds gravity itself through and demands zeroes.
        xh = (x * cosPitch) + (y * sinPitch * sinRoll) + (z * sinPitch * cosRoll);
        yh = (y * cosRoll) - (z * sinRoll);

        return true;
    }

    /// How far from edge-on the watch has to be for a projection to mean
    /// anything, as the square of cos(pitch). 0.03 is about 10 degrees of the
    /// watch face still facing up.
    static constexpr float MIN_COS_PITCH_SQ = 0.03f;

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
