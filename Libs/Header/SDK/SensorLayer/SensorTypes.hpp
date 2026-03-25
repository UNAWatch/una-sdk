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

#include <cstdint>

namespace SDK::Sensor
{
    enum class Type : uint32_t
    {
        UNKNOWN              = 0x00000000, ///< Unknown or invalid sensor type.

        /** @name IMU sensors
         *  @{
         */
        ACCELEROMETER        = 0x00000010, ///< Acceleration (3-axis).
        ACCELEROMETER_RAW    = 0x00000011, ///< Acceleration raw samples (implementation-defined units).

        GYROSCOPE            = 0x00000020, ///< Angular rate (3-axis).
        GYROSCOPE_RAW        = 0x00000021, ///< Angular rate raw samples.

        MAGNETIC_FIELD       = 0x00000030, ///< Magnetic field (3-axis).
        /** @} */

        /** @name Cardio sensors
         *  @{
         */
        HEART_BEAT           = 0x00000040, ///< Beat peak event.
        HEART_RATE           = 0x00000041, ///< Current heart rate (bpm).
        HEART_RATE_METRICS   = 0x00000042, ///< Aggregated metrics (e.g., AHR, RHR).
        /** @} */

        /** @name Pedometer
         *  @{
         */
        STEP_DETECTOR        = 0x00000050, ///< Step event.
        STEP_COUNTER         = 0x00000051, ///< Step count since last reboot.

        FLOOR_COUNTER        = 0x00000060, ///< Floor counter.
        /** @} */


        /** @name Ambient parameters
         *  @{
         */
        AMBIENT_TEMPERATURE  = 0x00000070, ///< Ambient temperature.
        PRESSURE             = 0x00000080, ///< Atmospheric pressure.
        ALTIMETER            = 0x00000090, ///< Altimeter.
        /** @} */


        /** @name Wrist motion detect
         *  @{
         */
        WRIST_MOTION         = 0x000000A0, ///< Wrist-motion event

        /** @name Motion / activity
         *  @{
         */
        MOTION_DETECT        = 0x000000B0, ///< Motion state events. NO_MOTION, MOTION, SIG_MOTION
        ACTIVITY_RECOGNITION = 0x000000C0, ///< Activity state classification. STILL, WALKING, RUNNING, UNKNOWN
        GESTURE_RECOGNITION  = 0x000000D0, ///< Discrete gesture events.
        /** @} */


        /** @name Daily activity
         *  @{
         */
        ACTIVITY             = 0x000000E0, ///< Active minutes for the current day (minutes).
        /** @} */


        /** @name Optical / bio signals
         *  @{
         */
        PPG                  = 0x000000F0, ///< Photoplethysmogram data.
        ECG                  = 0x00000100, ///< Electrocardiogram data.
        /** @} */

        /** @name GPS
         *  @{
         */
        GPS_LOCATION         = 0x00000110, ///< GNSS location.
        GPS_SPEED            = 0x00000111, ///< GNSS speed.
        GPS_DISTANCE         = 0x00000112, ///< GNSS distance / odometer.
        /** @} */

        /** @name Battery
         *  @{
         */
        BATTERY_LEVEL        = 0x00000120, ///< Charge level (%).
        BATTERY_CHARGING     = 0x00000121, ///< Charging state.
        BATTERY_METRICS      = 0x00000122, ///< Voltage/current/capacity metrics.
        /** @} */

        /** @name Fusion
         *  @{
         */
        FUSION               = 0x00000130, ///< Fused IMU (accel+gyro+mag). +
        FUSION_RAW           = 0x00000131, ///< Raw fusion inputs. +
        /** @} */

        /** @name Touch
         *  @{
         */
        TOUCH_DETECT         = 0x00000140, ///< Touch detection, worn / unworn
        /** @} */
    };
}
