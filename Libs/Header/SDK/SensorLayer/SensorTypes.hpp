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
    enum class Type : uint32_t {
        /*!< Unknown or invalid sensor type. */
        UNKNOWN              = 0x00000000,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes. */
        ACCELEROMETER        = 0x00000010,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes as int16_t. */
        ACCELEROMETER_RAW    = 0x00000020,

        /*!< A gyroscope sensor reports the rate of rotation of the device
         around the three sensor axes. */
        GYROSCOPE            = 0x00000030,

        /*!< A magnetic field sensor (also known as magnetometer) reports the
         ambient magnetic field, as measured along the three sensor axes. */
        MAGNETIC_FIELD       = 0x00000040,

        /*!< A heart rate sensor reports the current heart rate of the person
         touching the device. */
        HEART_RATE           = 0x00000050,  //// Heart rate
        HEART_RATE_METRICS   = 0x00000051,  //// Average value, RHR

        /*!< A sensor of this type returns the number of steps taken by the
          user since the last reboot while activated. */
        STEP_COUNTER         = 0x00000060,

        /*!< Ambient temperature sensor type. */
        AMBIENT_TEMPERATURE  = 0x00000070,

        /*!< Atmospheric pressure sensor type. */
        PRESSURE             = 0x00000080,

        /*!< Altimeter sensor type. */
        ALTIMETER            = 0x00000090,

        /*!< A virtual sensor that produces event if the device has been
         stationary for at least 5 seconds with a maximal latency of 5 additional
         seconds. ie: it may take up anywhere from 5 to 10 seconds after the
         device has been at rest to trigger this event. */
        STATIONARY_DETECT    = 0x000000A0,

        /*!< Motion detection virtual sensor event types
         This virtual sensor reports one of the following event types:

         - 0: No motion — device has stopped moving after previous motion.
         - 1: Motion — device has started moving (continuous motion detected).
         - 2: Significant motion — sudden or strong motion event.
         Typical usage includes detecting activity changes (e.g. start walking),
         power-saving transitions, or triggering other sensor logic.*/
        MOTION_DETECT        = 0x000000B0,

        /*!< A sensor of this type returns an event everytime a heart beat peak
         is detected. Peak here ideally corresponds to the positive peak in
         the QRS complex of an ECG signal.*/
        HEART_BEAT           = 0x000000C0,

        /*!<  */
        PPG                  = 0x000000D0,

        /*!<  */
        ECG                  = 0x000000E0,

        /*!< GPS Location */
        GPS_LOCATION         = 0x000000F0,

        /*!< GPS Speed */
        GPS_SPEED            = 0x00000100,

        /*!< GPS Distance*/
        GPS_DISTANCE         = 0x00000110,

        /*!< Floor counter */
        FLOOR_COUNTER        = 0x00000120,

        /*!< Activity */
        ACTIVITY             = 0x00000130,

        /*!< A sensor of this type returns an event every time a step is detected.
             Each event corresponds to a single human step, similar to how
             HEART_BEAT represents a single heartbeat peak. */
        STEP_DETECTOR        = 0x00000140,

        /*!< Activity recognition virtual sensor event types
         This virtual sensor detects and reports the user's current physical activity.

         Supported activities include:
         - STILL — user is not moving
         - WALKING — user is walking
         - RUNNING — user is running

         Activity events represent ongoing states and may change over time.*/
        ACTIVITY_RECOGNITION = 0x00000150,

        /*!< Gesture recognition virtual sensor
         This virtual sensor detects and reports discrete user gestures.

         Gesture events are triggered immediately upon recognition.
         The specific gesture types are implementation-defined. */
        GESTURE_RECOGNITION  = 0x00000160,

        /*!< Battery level sensor reports the charge level in percentage */
        BATTERY_LEVEL        = 0x00000170,

        /*!< Battery charge sensor tells you whether charging is currently taking place or not.*/
        BATTERY_CHARGING     = 0x00000180,

        /*!< Battery telemetry sensor reports voltage, current and capacity values.*/
        BATTERY_METRICS      = 0x00000190,
    };
}
