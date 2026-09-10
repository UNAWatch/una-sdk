/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   Console clockface service: the clock and the day's step count.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include <cstdint>
#include <ctime>

/**
 * @class Service
 * @brief Background half of the app: the clock and one sensor.
 *
 * The leanest of the four faces. It draws no charge level, no active minutes
 * and no heart rate, so it subscribes to none of them -- which for heart rate
 * matters beyond tidiness, since that subscription is what keeps the optical
 * sensor running.
 *
 * The clock is read once a turn round the loop, which is both what gets
 * published and what sizes the wait; the step sensor is event driven, so
 * apart from its events the thread is blocked.
 */
class Service
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    void connect();
    void disconnect();

    /** Parse one sensor-layer batch and hand the reading on. */
    void handleSensorData(uint16_t handle, SDK::Sensor::DataBatch &data);

    /**
     * @brief Re-read the kernel's system settings and publish what changed.
     *
     * The settings are pull-only -- nothing in the SDK reports a change -- so
     * this is called on each of the three occasions that can follow one: the
     * GUI starting, the GUI asking after a resume, and the loop's own poll.
     * The poll is bounded to once a minute and is what catches a format
     * pushed from the phone while the face is on screen, which no event
     * announces and neither lifecycle edge can see.
     */
    void refreshSystemSettings();

    /**
     * @brief Re-send everything the GUI draws, whether or not it has changed.
     *
     * The publishers below drop a value equal to the one they last *delivered*,
     * which is right for a steady stream but not enough after a suspension:
     * the GUI's custom-message queue is ten deep, a suspended GUI never drains
     * it, and a full queue rejects the newest message outright. A publish that
     * failed that way is retried -- the flag records the send's result -- but
     * only when its source next speaks, and a charge level or a step count can
     * be quiet for many minutes. Clearing the flags here forces the current
     * value out on resume instead of waiting for one, which is what makes
     * @ref Refresh mean what it says.
     */
    void republishAll();

    /** Send the reading on, unless it matches the one last delivered. */
    void publishTime(const std::tm &local);

    /** @brief Tell the GUI the day's step count. */
    void publishSteps();

    /** @brief Tell the GUI whether the watch is on a 12-hour clock. */
    void publishClockFormat();

    SDK::Kernel            &mKernel;
    SDK::Sensor::Connection mStepSensor;    ///< Steps today, event driven

    uint8_t  mHour;                 ///< Last reading sent to the GUI, compared
    uint8_t  mMinute;               ///< in full, because a clock can be set to
    uint8_t  mMday;                 ///< the same time on a different day
    uint8_t  mWday;
    uint8_t  mMon;
    bool     mTimeSent;             ///< A time has reached the GUI

    uint32_t mSteps;                ///< Latest reading from the sensor layer
    uint32_t mSentSteps;            ///< Last count sent to the GUI
    bool     mStepsSent;            ///< A count has reached the GUI

    uint32_t mSettingsAt;           ///< Monotonic tick of the last settings read
    bool     mIs12h;                ///< Clock format as last read from settings
    bool     mSentIs12h;            ///< Last format sent to the GUI
    bool     mFormatSent;           ///< A format has reached the GUI
};

#endif // SERVICE_HPP
