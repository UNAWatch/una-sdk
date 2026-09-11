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
 * gets published and what sets the next minute boundary. The wait is that
 * boundary or a pending heart-rate expiry, whichever falls sooner. The four
 * sensors it subscribes to are all event driven, so apart from those the
 * thread is blocked.
 *
 * The clock format and the date order are the two things here that are neither
 * a clock reading nor a sensor event: they are pulled from the kernel's system
 * settings, which push nothing when they change. See
 * @ref refreshSystemSettings.
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
     * The poll is bounded to once a minute and is what catches a change made
     * while the face is on screen, which no event announces and neither
     * lifecycle edge can see. The kernel re-reads settings.json when the phone
     * finishes writing it over BLE, and local_settings.json -- which is where
     * the clock format and the date order live -- when a USB session ends.
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

    /**
     * @brief Drop the held heart rate once it has aged out.
     *
     * Called every turn round the loop rather than only when a sample arrives,
     * because the case that matters most -- the watch taken off -- is the one
     * where samples stop coming.
     */
    void expireHeartRate();

    /** Send the reading on, unless it matches the one last delivered. */
    void publishTime(const std::tm &local);

    // Everything the face shows leaves through one of these. Each drops a
    // value equal to the one it last delivered, so a source may call them as
    // often as it likes and only a real change costs an IPC round trip.
    //
    // Delivered, not merely attempted: each takes its sent flag from
    // send_msg's result, because that fails when the GUI's queue is full and a
    // value recorded as sent would never be offered again.

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
     * @brief Whether a heart-rate sample is worth believing.
     *
     * The parser's own isDataValid() only checks the field count, so it says
     * nothing about signal quality. The predicate is the activity apps' -- a
     * trust level of 0 means no signal, and a bpm at or below 20 is not a
     * human resting rate -- but note what they use it for: deciding whether a
     * sample reaches a FIT file. It is not a display filter. Applied per
     * sample to a row on screen it blanks it on every dip that wrist movement
     * causes, which is the defect this face shipped with.
     *
     * So a sample that fails here is treated as no new information rather than
     * no reading: the held rate stands, and @ref expireHeartRate is what
     * eventually gives up on it.
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
    uint32_t mBpmAt;                ///< Monotonic tick when that rate was read
    uint32_t mSentSteps;            ///< Last health triple sent to the GUI
    uint32_t mSentActivityMinutes;
    uint16_t mSentBpm;
    bool     mHealthSent;           ///< A health triple has reached the GUI

    uint32_t mSettingsAt;           ///< Monotonic tick of the last settings read
    bool     mIs12h;                ///< Clock format as last read from settings
    bool     mMonthFirst;           ///< Date order as last read from settings
    bool     mSentIs12h;            ///< Last format sent to the GUI
    bool     mSentMonthFirst;       ///< Last date order sent to the GUI
    bool     mFormatSent;           ///< A format has reached the GUI

    bool     mMuted;                ///< Last mute state sent to the GUI
    bool     mMutedSent;            ///< A mute state has reached the GUI
};

#endif // SERVICE_HPP
