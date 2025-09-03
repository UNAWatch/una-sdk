#ifndef __INTERFACE_I_SENSOR_HPP
#define __INTERFACE_I_SENSOR_HPP

#include <cstdint>
#include <vector>

namespace Interface
{


class Sensor {

    enum Type : int32_t {
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

    /**
     * @brief Accelerometer event data.
     */
    struct AccelerometerEventData {
        // All values are in SI units (m/s^2)
        union {
            float v[3];
            struct {
                float x;
                float y;
                float z;
            };
        };
    };

    /**
     * @brief Gyroscope  event data.
     */
    struct GyroscopeEventData {
        // All values are in radians per second (rad/s).
        union {
            float v[3];
            struct {
                float x;
                float y;
                float z;
            };
        };
    };

    /**
     * @brief Magnetic field event data.
     */
    struct MagneticFieldEventData {
        // All values are in micro-Tesla (uT).
        union {
            float v[3];
            struct {
                float x;
                float y;
                float z;
            };
        };
    };

    /**
     * Heart rate event data
     */
    struct HeartRateEventData {
        /*!< Heart rate in beats per minute.
         Set to 0 when status is ..._UNRELIABLE or ..._NO_CONTACT */
        float bpm;

        /*!< Status of the sensor for this reading. Set to one HR_STATUS_...
         Note that this value should only be set for sensors that explicitly
         define the meaning of this field. */
        int8_t status;
    };

    /**
     * @brief Step counter event data
     */
    struct StepCounterEventData {
        /// The number of steps taken by the user since the last reboot while activated.
        uint32_t value;
    };

    /**
     * @brief Temperature (barometer) event data.
     */
    struct TemperatureEventData {
        /// Ambient (room) temperature in degrees Celsius.
        float value;
    };

    /**
     * @brief Pressure (barometer) event data.
     */
    struct PressureEventData {
        /// The atmospheric pressure in hPa (millibar).
        float value;
    };

    /**
     * @brief Stationary detect event data.
     */
    struct StationaryEventData {
        /// Allowed only true.
        bool value;
    };

    /**
     * @brief Motion detect event data.
     */
    struct MotionEventData {
        /// Allowed only true.
        bool value;
    };

    /**
     * @brief Heart beat event data
     */
    struct HeartBeatEventData {
        /*<! A confidence value of 0.0 indicates complete uncertainty - that a peak
         is as likely to be at the indicated timestamp as anywhere else.
         A confidence value of 1.0 indicates complete certainly - that a peak
         is completely unlikely to be anywhere else on the QRS complex. */
        float confidence;
    };

    /**
     * @brief PPG sensor event data.
     */
    struct PPGEventData {

    };

    /**
     * @brief ECG sensor event data.
     */
    struct ECGEventData {

    };

};






class SensorEvent {
    /// Set to true when this is the first sensor event after a discontinuity.
    bool firstEventAfterDiscontinuity;

    /// The sensor that generated this event.
    Sensor* pSensor;

    /*!< The time in milliseconds at which the event happened.
         Time since system start (after reboot). */
    uint32_t timestamp;


    union {

    };

};

class SensorEventListener {
public:

    /**
     * @brief Called when there is a new sensor event or batch  of events
     *        in case of using hardware or software FIFO.
     * @note This will also be called if we have a new reading from a sensor
     *       with the exact same sensor values (but a newer timestamp).
     * @note The application doesn't own the event object passed as a parameter
     *       and therefore cannot hold on to it. The object may be part of an
     *       internal pool and may be reused by the framework.
     * @param pEvents: pointer to the SensorEvent array.
     * @param eventsNum: The number of events in array.
     */
    virtual void onSensorChanged(const SensorEvent *pEvents, uint32_t eventsNum) = 0;
};

class SensorManager {
public:

    /**
     * @brief Use this method to get the default sensor for a given type.
     * @note Returned sensor could be a composite sensor, and its data could
     *       be averaged or filtered. If you need to access the raw sensors
     *       use getSensorList.
     * @param type: Sensor requested type.
     * @return The default sensor matching the requested type if one exists,
     *         or null otherwise.
     */
    Sensor* getDefaultSensor(int32_t type);

    /**
     * @brief Get the list of available sensors of a certain type.
     * @param type: Sensor requested type.
     * @return A list of sensors matching the asked type.
     */
    const std::vector<Sensor*>& getSensorList(int32_t type);

    /**
     * @brief Registers a SensorEventListener for the given sensor at the given
     *        sampling frequency and the given maximum reporting latency.
     * @param listener: A SensorEventListener object that will receive the
     *        sensor events.
     * @param pSensor: The Sensor to register to.
     * @param samplingPeriodMs: The desired delay between two consecutive
     *        events in milliseconds. This is only a hint to the system. Events
     *        may be received faster or slower than the specified rate. Usually
     *        events are received faster.
     *        The parameter is ignored for one-shot sensors.
     *        Defaul value is 0 - maximum sampling rate, which depends on the
     *        hardware.
     * @param maxReportLatencyMs: Maximum time in milliseconds that events can
     *        be delayed before being reported to the application. A large value
     *        allows reducing the power consumption associated with the sensor.
     *        If maxReportLatencyMs is set to zero, events are delivered as
     *        soon as they are available.
     *        Default value is 1000 ms.
     * @return 'true' if the sensor is supported and successfully enabled.
     */
    bool registerListener(SensorEventListener &listener, Sensor *pSensor,
            uint32_t samplingPeriodMs = 0, uint32_t maxReportLatencyMs = 1000);

    /**
     * @brief Unregisters a listener.
     * @param listener: A SensorListener object.
     * @param pSensor: The sensor to unregister from. The sensor to unregister
     *        from. If nullpt, then unregisters a listener all sensors.
     */
    void unregisterListener(SensorEventListener &listener, Sensor *pSensor);

    /**
     * @brief Flushes the FIFO of all the sensors registered for this listener.
     *        Events are returned in the usual way through the SensorEventListener.
     *        This call is asynchronous and returns immediately. onFlushCompleted
     *        is called after all the events in the batch at the time of calling
     *        this method have been delivered successfully.
     *        If the hardware doesn't support flush, it still returns true.
     * @param listener: A SensorEventListener object which was previously used
     *        in a registerListener call.
     * @return 'true' if the flush is initiated successfully on all the sensors
     *         registered for this listener, 'false' if no sensor is previously
     *         registered for this listener or flush on one of the sensors fails.
     */
    bool flush(SensorEventListener &listener);

};

} /* namespace Interface */

#endif /* __INTERFACE_I_SENSOR_HPP */
