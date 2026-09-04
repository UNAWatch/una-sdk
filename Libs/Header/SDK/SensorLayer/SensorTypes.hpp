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

        MAGNETIC_FIELD       = 0x00000030, ///< Magnetic field (3-axis) + MAG_CALIBRATED; corrected only when that flag is set.
        MAGNETIC_FIELD_RAW   = 0x00000031, ///< Magnetic field (3-axis) exactly as measured, with no correction applied.
        /** @} */

        /** @name Cardio sensors
         *  @{
         */
        HEART_BEAT           = 0x00000040, ///< Beat peak event.
        HEART_RATE           = 0x00000041, ///< Current (arbitrated) heart rate (bpm) + trust. 2 fields.
        HEART_RATE_METRICS_DAILY = 0x00000042, ///< Aggregated metrics for the current day (e.g., AHR, RHR).
        HEART_RATE_METRICS   = HEART_RATE_METRICS_DAILY, /// Legacy alias of HEART_RATE_METRICS_DAILY.
        HEART_RATE_EX        = 0x00000043, ///< Opt-in multi-source HR: arbitrated + source + raw optical + raw external. 7 fields.
        /** @} */

        /** @name Pedometer
         *  @{
         */
        STEP_DETECTOR        = 0x00000050, ///< Step event.
        STEP_COUNTER         = 0x00000051, ///< Step count since boot (monotonic).
        STEP_COUNTER_DAILY   = 0x00000052, ///< Step count for the current day.
        RUNNING_CADENCE      = 0x00000053, ///< Running cadence (steps/min); step length is derived SDK-side.

        FLOOR_COUNTER        = 0x00000060, ///< Floor counter since boot (monotonic).
        FLOOR_COUNTER_DAILY  = 0x00000061, ///< Floor counter for the current day.
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
        ACTIVITY_TIME        = 0x000000E0, ///< Active minutes since boot (monotonic).
        ACTIVITY_TIME_DAILY  = 0x000000E1, ///< Active minutes for the current day.
        ACTIVITY             = ACTIVITY_TIME_DAILY, ///< Active minutes for the current day (minutes). Legacy alias of ACTIVITY_TIME_DAILY.
        /** @} */


        /** @name Optical / bio signals
         *  @{
         */
        PPG                  = 0x000000F0, ///< Photoplethysmogram data.
        SPO2                 = 0x000000F1, ///< Blood-oxygen saturation (SpO2), derived from the optical PPG path (%).
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

        /** @name Derived terrain
         *  @{
         */
        GRADE                = 0x00000150, ///< Barometric terrain grade (%) + validity.
        /** @} */
    };
}
