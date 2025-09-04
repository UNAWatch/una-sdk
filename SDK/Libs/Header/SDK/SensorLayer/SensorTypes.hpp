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

namespace SDK::Sensor
{
    enum class Type : int {
        /*!< Unknown or invalid sensor type. */
        UNKNOWN = -1,

        /*!< An accelerometer sensor reports the acceleration of the device
         along the three sensor axes. */
        ACCELEROMETER,

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

        /*!< A virtual sensor that produces event  if the device has been in
         motion for at least 5 seconds with a maximal latency of 5 additional
         seconds. ie: it may take up anywhere from 5 to 10 seconds after the
         device has been at rest to trigger this event. */
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
        FLOOR_COUNTER
    };

} /* namespace Sensor */

#endif /* __SENSOR_TYPES_HPP */
