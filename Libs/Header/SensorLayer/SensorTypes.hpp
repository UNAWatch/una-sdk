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

#ifndef __SENSOR_TYPES_HPP
#define __SENSOR_TYPES_HPP

namespace Sensor
{
    enum class Type : int {
        /*!< Unknown or invalid sensor type. */
        SENSOR_TYPE_UNKNOWN = -1,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes. */
        SENSOR_TYPE_ACCELEROMETER,

        /*!< A gyroscope sensor reports the rate of rotation of the device
         around the three sensor axes. */
        SENSOR_TYPE_GYROSCOPE,

        /*!< A magnetic field sensor (also known as magnetometer) reports the
         ambient magnetic field, as measured along the three sensor axes. */
        SENSOR_TYPE_MAGNETIC_FIELD,

        /*!< A heart rate sensor reports the current heart rate of the person
         touching the device. */
        SENSOR_TYPE_HEART_RATE,

        /*!< A sensor of this type returns the number of steps taken by the
          user since the last reboot while activated. */
        SENSOR_TYPE_STEP_COUNTER,

        /*!< Ambient temperature sensor type. */
        SENSOR_TYPE_AMBIENT_TEMPERATURE,

        /*!< Atmospheric pressure sensor type. */
        SENSOR_TYPE_PRESSURE,

        /*!< Altimeter sensor type. */
        SENSOR_TYPE_ALTIMETER,

        /*!< A virtual sensor that produces event if the device has been
         stationary for at least 5 seconds with a maximal latency of 5 additional
         seconds. ie: it may take up anywhere from 5 to 10 seconds after the
         device has been at rest to trigger this event. */
        SENSOR_TYPE_STATIONARY_DETECT,

        /*!< A virtual sensor that produces event  if the device has been in
         motion for at least 5 seconds with a maximal latency of 5 additional
         seconds. ie: it may take up anywhere from 5 to 10 seconds after the
         device has been at rest to trigger this event. */
        SENSOR_TYPE_MOTION_DETECT,

        /*!< A sensor of this type returns an event everytime a heart beat peak
         is detected. Peak here ideally corresponds to the positive peak in
         the QRS complex of an ECG signal.*/
        SENSOR_TYPE_HEART_BEAT,

        /*!<  */
        SENSOR_TYPE_PPG,

        /*!<  */
        SENSOR_TYPE_ECG,
    };

} /* namespace Sensor */

#endif /* __SENSOR_TYPES_HPP */
