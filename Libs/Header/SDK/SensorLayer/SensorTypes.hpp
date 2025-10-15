/**
 ******************************************************************************
 * @file    SensorTypes.hpp
 * @date    28-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Set of sensor types.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

namespace SDK::Sensor
{
    enum class Type : int {
        /*!< Unknown or invalid sensor type. */
        UNKNOWN = -1,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes. */
        ACCELEROMETER,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes as int16_t. */
        ACCELEROMETER_RAW,

        /*!< A gyroscope sensor reports the rate of rotation of the device
         around the three sensor axes. */
        GYROSCOPE,

        /*!< A magnetic field sensor (also known as magnetometer) reports the
         ambient magnetic field, as measured along the three sensor axes. */
        MAGNETIC_FIELD,

        /*!< A heart rate sensor reports the current heart rate of the person
         touching the device. */
        HEART_RATE,

        /*!< A sensor of this type returns the number of steps taken by the
          user since the last reboot while activated. */
        STEP_COUNTER,

        /*!< Ambient temperature sensor type. */
        AMBIENT_TEMPERATURE,

        /*!< Atmospheric pressure sensor type. */
        PRESSURE,

        /*!< Altimeter sensor type. */
        ALTIMETER,

        /*!< A virtual sensor that produces event if the device has been
         stationary for at least 5 seconds with a maximal latency of 5 additional
         seconds. ie: it may take up anywhere from 5 to 10 seconds after the
         device has been at rest to trigger this event. */
        STATIONARY_DETECT,

        /*!< Motion detection virtual sensor event types
         This virtual sensor reports one of the following event types:

         - 0: No motion — device has stopped moving after previous motion.
         - 1: Motion — device has started moving (continuous motion detected).
         - 2: Significant motion — sudden or strong motion event.
         Typical usage includes detecting activity changes (e.g. start walking),
         power-saving transitions, or triggering other sensor logic.*/
        MOTION_DETECT,

        /*!< A sensor of this type returns an event everytime a heart beat peak
         is detected. Peak here ideally corresponds to the positive peak in
         the QRS complex of an ECG signal.*/
        HEART_BEAT,

        /*!<  */
        PPG,

        /*!<  */
        ECG,

        /*!< GPS */
        GPS,

        /*!< Floor counter */
        FLOOR_COUNTER,

        /*!< Activity */
        ACTIVITY,

        /*!< A sensor of this type returns an event every time a step is detected.
             Each event corresponds to a single human step, similar to how
             HEART_BEAT represents a single heartbeat peak. */
        STEP_DETECTOR,

        /*!< Activity recognition virtual sensor event types
         This virtual sensor detects and reports the user's current physical activity.

         Supported activities include:
         - STILL — user is not moving
         - WALKING — user is walking
         - RUNNING — user is running

         Activity events represent ongoing states and may change over time.*/
        ACTIVITY_RECOGNITION,

        /*!< Gesture recognition virtual sensor
         This virtual sensor detects and reports discrete user gestures.

         Gesture events are triggered immediately upon recognition.
         The specific gesture types are implementation-defined. */
        GESTURE_RECOGNITION,
    };

} // namespace Sensor
