/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   Smile clockface service: the clock, the charge level and the day's
 *          three health readings.
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
 * @brief Background half of the app: it owns everything the face draws.
 *
 * The face has no second hand, so the fastest thing it draws turns once a
 * minute; the clock is read once a turn round the loop, which is both what
 * gets published and what sizes the wait. The four sensors it subscribes to
 * are all event driven, so between minute boundaries the thread is blocked.
 *
 * The clock format is the one thing here that is neither a clock reading nor a
 * sensor event: it is pulled from the kernel's system settings, which push
 * nothing when they change. See @ref refreshSystemSettings.
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
     * this is called on the two edges that can follow one: the GUI starting,
     * and the GUI asking after a resume. Between them nothing re-reads, which
     * is what keeps a face that is just telling the time off the kernel's
     * message queue.
     */
    void refreshSystemSettings();

    /**
     * @brief Re-send everything the GUI draws, whether or not it has changed.
     *
     * The publishers below all drop a value equal to the one they last sent,
     * which is right for a steady stream but wrong after a suspension: the
     * GUI's custom-message queue is ten deep, a suspended GUI never drains it,
     * and a full queue rejects the newest message outright. So a value that
     * moved while the face was off screen can be lost, and the publisher will
     * not offer it again. Clearing the sent flags is what makes @ref Refresh
     * mean what it says.
     */
    void republishAll();

    /**
     * @brief Drop the held heart rate once it has aged out.
     *
     * Called every turn round the loop rather than only when a sample arrives,
     * because the case that matters most -- the watch taken off -- is the one
     * where samples stop coming.
     */
    void expireHeartRate();

    /** Send the reading on, unless it matches the one last sent. */
    void publishTime(const std::tm &local);

    // Everything the face shows leaves through one of these. Each drops a
    // value equal to the one it last sent, so a source may call them as often
    // as it likes and only a real change costs an IPC round trip.

    /** @brief Tell the GUI the charge level, 0-100. */
    void publishBatteryLevel(uint8_t level);

    /** @brief Tell the GUI the day's steps, active minutes and heart rate. */
    void publishHealth();

    /** @brief Tell the GUI whether the watch is on a 12-hour clock. */
    void publishClockFormat();

    /**
     * @brief Tell the GUI whether alerts are currently silenced.
     *
     * Nothing calls this, and that is the whole story: the SDK exposes no mute
     * state for an app to read. The path is here because the message and the
     * GUI's handling of it are, and attaching a source is all that would
     * remain if one appeared.
     */
    void publishAlertsMuted(bool muted);

    /**
     * @brief Whether a heart-rate sample is worth showing.
     *
     * The parser's own isDataValid() only checks the field count, so it says
     * nothing about signal quality. The activity apps all gate on the same
     * pair of conditions, and this face follows them: a trust level of 0 means
     * no signal, and a bpm at or below 20 is not a human resting rate.
     */
    static bool isHeartRateTrusted(float bpm, float trustLevel);

    SDK::Kernel            &mKernel;
    SDK::Sensor::Connection mBatterySensor;   ///< Charge level, event driven
    SDK::Sensor::Connection mStepSensor;      ///< Steps today, event driven
    SDK::Sensor::Connection mActivitySensor;  ///< Active minutes today
    SDK::Sensor::Connection mHeartRateSensor; ///< Arbitrated heart rate

    uint8_t  mHour;                 ///< Last reading sent to the GUI, compared
    uint8_t  mMinute;               ///< in full, because a clock can be set to
    uint8_t  mMday;                 ///< the same time on a different day
    uint8_t  mWday;
    uint8_t  mMon;
    bool     mTimeSent;             ///< A time has reached the GUI

    uint8_t  mLevel;                ///< Last level sent to the GUI
    bool     mLevelSent;            ///< A level has reached the GUI

    uint32_t mSteps;                ///< Latest reading from the sensor layer
    uint32_t mActivityMinutes;
    uint16_t mBpm;                  ///< Last trusted rate, 0 once given up on
    std::time_t mBpmAt;             ///< When that rate was read, for the hold
    uint32_t mSentSteps;            ///< Last health triple sent to the GUI
    uint32_t mSentActivityMinutes;
    uint16_t mSentBpm;
    bool     mHealthSent;           ///< A health triple has reached the GUI

    bool     mIs12h;                ///< Clock format as last read from settings
    bool     mSentIs12h;            ///< Last format sent to the GUI
    bool     mFormatSent;           ///< A format has reached the GUI

    bool     mMuted;                ///< Last mute state sent to the GUI
    bool     mMutedSent;            ///< A mute state has reached the GUI
};

#endif // SERVICE_HPP
