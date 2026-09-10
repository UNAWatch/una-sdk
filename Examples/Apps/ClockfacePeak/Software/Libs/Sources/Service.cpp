#include "Service.hpp"
#include "Commands.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"

#include <ctime>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

static constexpr uint32_t kSecondsPerMinute = 60;
static constexpr uint32_t kMsPerSecond      = 1000;

/// How long to wait for the kernel to answer a settings request. The same
/// 100 ms every other app that reads them uses. A timeout, not a cost: the
/// kernel answers on a completion semaphore and normally returns at once.
static constexpr uint32_t kSettingsTimeoutMs = 100;

/// How often to re-read the system settings unprompted.
///
/// They are pull-only, and the two lifecycle edges the service reads them on
/// cover the local route to changing them -- Settings suspends the face, so
/// returning to it resumes and asks. What they do not cover is a change pushed
/// from the phone while the face is on screen, which no event announces. A
/// minute is far more often than a user can plausibly change the setting, and
/// costs one request whose result the publishers drop when nothing moved.
static constexpr uint32_t kSettingsPollMs = 60u * kMsPerSecond;

/// Below this the reading is not a human heart rate, whatever the sensor says.
static constexpr float kMinPlausibleBpm = 20.0f;

/// Trust runs 0-3, and 0 means no signal.
static constexpr float kMinTrustLevel = 1.0f;
static constexpr float kMaxTrustLevel = 3.0f;

/// How long the last trusted rate stays on screen once trust is lost. Long
/// enough to ride out the dips that wrist movement causes, short enough that a
/// watch taken off does not keep showing a pulse.
///
/// Measured against the monotonic OS tick, not the wall clock: the wall clock
/// is set from the phone over BLE and can jump either way, which would strand a
/// reading on screen indefinitely on a backward jump and retire one early on a
/// forward one.
static constexpr uint32_t kHeartRateHoldMs = 10u * kMsPerSecond;

/** @brief Read the local time, to the minute. */
static void readLocalTime(std::tm &out)
{
    std::time_t utc = time(nullptr);

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&out, &utc);
#else
    localtime_r(&utc, &out);
#endif
}

/** @brief How long until the held heart rate ages out, or zero if none is held. */
static uint32_t msToHeartRateExpiry(uint16_t bpm, uint32_t heldAt, uint32_t now)
{
    if (bpm == 0u) {
        return 0;
    }

    // Unsigned, so it stays right across the tick's wrap at ~49 days.
    const uint32_t elapsed = now - heldAt;
    if (elapsed >= kHeartRateHoldMs) {
        return 0;
    }

    return kHeartRateHoldMs - elapsed;
}

/** @brief How much of the current minute is left, from a reading already taken. */
static uint32_t msToNextMinute(const std::tm &local)
{
    const uint32_t sec = static_cast<uint32_t>(local.tm_sec) % kSecondsPerMinute;

    // Seconds are the finest the reading carries, so this lands somewhere in
    // the first second of the new minute rather than exactly on it, and never
    // returns zero.
    return (kSecondsPerMinute - sec) * kMsPerSecond;
}

Service::Service(SDK::Kernel &kernel)
    : mKernel(kernel)
    , mBatterySensor(SDK::Sensor::Type::BATTERY_LEVEL)
    , mStepSensor(SDK::Sensor::Type::STEP_COUNTER_DAILY)
    , mActivitySensor(SDK::Sensor::Type::ACTIVITY_TIME_DAILY)
    , mHeartRateSensor(SDK::Sensor::Type::HEART_RATE)
    , mHour(0)
    , mMinute(0)
    , mMday(0)
    , mWday(0)
    , mMon(0)
    , mTimeSent(false)
    , mLevel(0)
    , mLevelSent(false)
    , mSteps(0)
    , mActivityMinutes(0)
    , mBpm(0)
    , mBpmAt(0)
    , mSentSteps(0)
    , mSentActivityMinutes(0)
    , mSentBpm(0)
    , mHealthSent(false)
    , mSettingsAt(0)
    , mIs12h(false)
    , mSentIs12h(false)
    , mFormatSent(false)
    , mStepsGoal(0)
    , mActivityGoal(0)
    , mSentStepsGoal(0)
    , mSentActivityGoal(0)
    , mGoalsSent(false)
    , mMuted(false)
    , mMutedSent(false)
{
}

Service::~Service()
{
    disconnect();
}

void Service::run()
{
    LOG_INFO("Started\n");

    // The face is on screen for as long as the app is loaded, so there is no
    // moment worth deferring the subscriptions to. Connecting publishes each
    // current reading straight away, which is what fills the face on boot.
    connect();

    while (true) {
        // One reading a turn, and it does both jobs: it is what gets published
        // and it is what sizes the wait. Publishing here rather than on the
        // wait expiring is what stops a message that arrives just before a
        // boundary from swallowing that minute -- publishTime() drops a
        // reading equal to the last, so an early turn costs nothing.
        std::tm local {};
        readLocalTime(local);
        publishTime(local);

        // The hold has to be able to expire with no further sample to notice
        // it: a watch taken off stops producing them altogether, and the rate
        // would otherwise stay on screen for good.
        expireHeartRate();

        // Wait no longer than the pending expiry, so it lands on time rather
        // than whenever the minute next turns. While the watch is worn samples
        // arrive far more often than this and keep resetting it, so it costs no
        // extra wake-ups in the common case.
        uint32_t wait = msToNextMinute(local);
        const uint32_t hold = msToHeartRateExpiry(mBpm, mBpmAt,
                                                  mKernel.sys.getTimeMs());
        if ((hold != 0u) && (hold < wait)) {
            wait = hold;
        }

        // Bounded against the monotonic tick rather than hung off the wait
        // expiring: the loop is message driven, and on a face showing a heart
        // rate the timeout branch almost never runs.
        if ((mKernel.sys.getTimeMs() - mSettingsAt) >= kSettingsPollMs) {
            refreshSystemSettings();
        }

        SDK::MessageBase *msg;
        if (!mKernel.comm.getMessage(msg, wait)) {
            continue;
        }

        bool done = false;

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto *event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                handleSensorData(event->handle, batch);
            } break;

            // The GUI reads the clock itself when its Model is built and the
            // sensors published on connect, so the only thing it cannot know
            // yet is the clock format.
            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                refreshSystemSettings();
                break;

            // The GUI resumed. It has been off screen, possibly across a
            // change to the very setting it cannot be told about.
            case CustomMessage::REFRESH:
                republishAll();
                break;

            case SDK::MessageType::COMMAND_APP_STOP:
            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                done = true;
                break;

            default:
                break;
        }

        mKernel.comm.releaseMessage(msg);

        // The kernel leaves a service running after its GUI closes so that it
        // can carry state across. This one has none, so it takes the chance to
        // drop the subscriptions and release its thread instead.
        if (done) {
            LOG_INFO("Exiting\n");
            disconnect();
            return;
        }
    }
}

void Service::connect()
{
    if (!mBatterySensor.isConnected()) {
        mBatterySensor.connect();
    }
    if (!mStepSensor.isConnected()) {
        mStepSensor.connect();
    }
    if (!mActivitySensor.isConnected()) {
        mActivitySensor.connect();
    }
    if (!mHeartRateSensor.isConnected()) {
        mHeartRateSensor.connect();
    }
}

void Service::disconnect()
{
    if (mBatterySensor.isConnected()) {
        mBatterySensor.disconnect();
    }
    if (mStepSensor.isConnected()) {
        mStepSensor.disconnect();
    }
    if (mActivitySensor.isConnected()) {
        mActivitySensor.disconnect();
    }
    if (mHeartRateSensor.isConnected()) {
        mHeartRateSensor.disconnect();
    }
}

bool Service::isHeartRateTrusted(float bpm, float trustLevel)
{
    return (bpm > kMinPlausibleBpm) &&
           (trustLevel >= kMinTrustLevel) &&
           (trustLevel <= kMaxTrustLevel);
}

void Service::expireHeartRate()
{
    if ((mBpm != 0u) &&
        ((mKernel.sys.getTimeMs() - mBpmAt) >= kHeartRateHoldMs)) {
        mBpm = 0;
        publishHealth();
    }
}

void Service::handleSensorData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    // The batch is built here out of fields the event arrived with, and
    // DataBatch guards its index with an assert, which a release build drops.
    // So the size is checked rather than assumed, and the newest sample is the
    // one read: a batch holds its samples oldest first, and only the last of
    // them is the reading as it stands now.
    if (data.size() == 0) {
        return;
    }

    const SDK::Sensor::DataView newest = data[data.size() - 1];

    if (mBatterySensor.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel parser(newest);
        if (parser.isDataValid()) {
            publishBatteryLevel(static_cast<uint8_t>(parser.getCharge()));
        }
        return;
    }

    if (mStepSensor.matchesDriver(handle)) {
        SDK::SensorDataParser::StepCounter parser(newest);
        if (parser.isDataValid()) {
            mSteps = parser.getStepCount();
            publishHealth();
        }
        return;
    }

    if (mActivitySensor.matchesDriver(handle)) {
        SDK::SensorDataParser::Activity parser(newest);
        if (parser.isDataValid()) {
            mActivityMinutes = parser.getDuration();
            publishHealth();
        }
        return;
    }

    if (mHeartRateSensor.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRate parser(newest);
        if (!parser.isDataValid()) {
            return;
        }

        const float bpm   = parser.getBpm();
        const float trust = parser.getTrustLevel();

        // Trust dips transiently whenever the wrist moves, so a sample that
        // fails the gate means "no new information", not "no rate". Blanking
        // on it makes the row flicker to --- for a second at every dip, which
        // is what an earlier version of this face did on the wrist.
        //
        // So the last good reading is held, and only a sustained loss gives up
        // on it. Holding is the right way round to be wrong here: a rate a few
        // seconds old still reads true, whereas a row that blinks reads broken.
        if (isHeartRateTrusted(bpm, trust)) {
            mBpm = static_cast<uint16_t>(bpm);
            mBpmAt = mKernel.sys.getTimeMs();
        }

        // An untrusted sample carries no information, so it neither updates
        // the held rate nor retires it. expireHeartRate() owns that, and runs
        // every turn round the loop whether a sample arrived or not. Before the
        // first trusted reading there is nothing to hold, so the row shows ---.

        expireHeartRate();
        publishHealth();
        return;
    }
}

void Service::republishAll()
{
    mTimeSent   = false;
    mLevelSent  = false;
    mHealthSent = false;
    mFormatSent = false;
    mGoalsSent  = false;

    // The clock is republished by the loop's next turn, which is immediate.
    publishBatteryLevel(mLevel);
    publishHealth();
    refreshSystemSettings();
}

void Service::refreshSystemSettings()
{
    // Stamped whether or not the read succeeds, so a kernel that is not
    // answering is retried on the next poll rather than on every turn.
    mSettingsAt = mKernel.sys.getTimeMs();

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(kSettingsTimeoutMs) && msg.ok()) {
            mIs12h        = msg->timeFormat;
            mStepsGoal    = msg->steps;
            mActivityGoal = msg->activityMin;
        }
    }

    // Published unconditionally rather than only on a successful read: the
    // publishers drop an unchanged value anyway, and a failed read leaves what
    // was last known, which is still the best answer there is. What this does
    // guarantee is that the GUI hears a format and a pair of targets at least
    // once, even if the very first request times out.
    publishClockFormat();
    publishGoals();
}

void Service::publishTime(const std::tm &local)
{
    const uint8_t hour   = static_cast<uint8_t>(local.tm_hour);
    const uint8_t minute = static_cast<uint8_t>(local.tm_min);
    const uint8_t mday   = static_cast<uint8_t>(local.tm_mday);
    const uint8_t wday   = static_cast<uint8_t>(local.tm_wday);
    const uint8_t mon    = static_cast<uint8_t>(local.tm_mon);

    if (mTimeSent && (hour == mHour) && (minute == mMinute) &&
        (mday == mMday) && (wday == mWday) && (mon == mMon)) {
        return;
    }

    mHour     = hour;
    mMinute   = minute;
    mMday     = mday;
    mWday     = wday;
    mMon      = mon;
    // Set from the result, not before it: send_msg fails when the GUI's
    // queue is full, and a value recorded as sent is never offered again.
    mTimeSent = SDK::send_msg<CustomMessage::Time>(mKernel, hour, minute, mday, wday, mon);
}

void Service::publishBatteryLevel(uint8_t level)
{
    // The indicator resolves to 25 % bands, so a fractional move is not worth
    // an IPC round trip; whole percent is already finer than it can show.
    if (mLevelSent && (level == mLevel)) {
        return;
    }

    mLevel     = level;
    mLevelSent = SDK::send_msg<CustomMessage::Battery>(mKernel, mLevel);
}

void Service::publishHealth()
{
    if (mHealthSent && (mSteps == mSentSteps) &&
        (mActivityMinutes == mSentActivityMinutes) && (mBpm == mSentBpm)) {
        return;
    }

    mSentSteps           = mSteps;
    mSentActivityMinutes = mActivityMinutes;
    mSentBpm             = mBpm;
    mHealthSent          = SDK::send_msg<CustomMessage::Health>(
        mKernel, mSentSteps, mSentActivityMinutes, mSentBpm);
}

void Service::publishClockFormat()
{
    if (mFormatSent && (mIs12h == mSentIs12h)) {
        return;
    }

    mSentIs12h  = mIs12h;
    mFormatSent = SDK::send_msg<CustomMessage::ClockFormat>(mKernel, mSentIs12h);
}

void Service::publishGoals()
{
    if (mGoalsSent && (mStepsGoal == mSentStepsGoal) &&
        (mActivityGoal == mSentActivityGoal)) {
        return;
    }

    mSentStepsGoal    = mStepsGoal;
    mSentActivityGoal = mActivityGoal;
    mGoalsSent        = SDK::send_msg<CustomMessage::Goals>(
        mKernel, mSentStepsGoal, mSentActivityGoal);
}

void Service::publishAlertsMuted(bool muted)
{
    if (mMutedSent && (muted == mMuted)) {
        return;
    }

    mMuted     = muted;
    mMutedSent = SDK::send_msg<CustomMessage::AlertsMuted>(mKernel, mMuted);
}
