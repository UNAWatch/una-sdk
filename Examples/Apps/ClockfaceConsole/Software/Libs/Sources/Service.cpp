#include "Service.hpp"
#include "Commands.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"

#include <ctime>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

static constexpr uint32_t kSecondsPerMinute = 60;
static constexpr uint32_t kMsPerSecond      = 1000;

/// Grace period after launch for the GUI to come up. The service is started
/// just before its GUI, so the no-GUI exit in run() must not trip during a
/// normal launch.
static constexpr uint32_t kStartupGraceMs = 5u * kMsPerSecond;

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
    , mStepSensor(SDK::Sensor::Type::STEP_COUNTER_DAILY)
    , mHour(0)
    , mMinute(0)
    , mMday(0)
    , mWday(0)
    , mMon(0)
    , mTimeSent(false)
    , mSteps(0)
    , mSentSteps(0)
    , mStepsSent(false)
    , mSettingsAt(0)
    , mIs12h(false)
    , mMonthFirst(false)
    , mSentIs12h(false)
    , mSentMonthFirst(false)
    , mFormatSent(false)
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
    // moment worth deferring the subscription to. Connecting publishes the
    // current count straight away, which is what fills the row on boot.
    connect();

    bool guiStarted = false;
    const uint32_t startTime = mKernel.sys.getTimeMs();

    while (true) {
        // One reading a turn, and it does both jobs: it is what gets published
        // and it is what sizes the wait. Publishing here rather than on the
        // wait expiring is what stops a message that arrives just before a
        // boundary from swallowing that minute -- publishTime() drops a
        // reading equal to the last, so an early turn costs nothing.
        std::tm local {};
        readLocalTime(local);
        publishTime(local);

        // Bounded against the monotonic tick rather than hung off the wait
        // expiring: the loop is message driven, and on a face showing a heart
        // rate the timeout branch almost never runs.
        if ((mKernel.sys.getTimeMs() - mSettingsAt) >= kSettingsPollMs) {
            refreshSystemSettings();
        }

        uint32_t wait = msToNextMinute(local);

        // No GUI yet. One that has not come up by the end of the grace never
        // will, and neither will the COMMAND_APP_NOTIF_GUI_STOP that normally
        // ends this service -- so leave once the grace has run out.
        if (!guiStarted) {
            const uint32_t elapsed = mKernel.sys.getTimeMs() - startTime;
            if (elapsed >= kStartupGraceMs) {
                LOG_INFO("GUI never started, exiting service\n");
                disconnect();
                return;
            }
            if ((kStartupGraceMs - elapsed) < wait) {
                wait = kStartupGraceMs - elapsed;
            }
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
            // step count was published on connect, so the only thing it cannot
            // know yet is the clock format.
            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                guiStarted = true;
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
        // drop the subscription and release its thread instead.
        if (done) {
            LOG_INFO("Exiting\n");
            disconnect();
            return;
        }
    }
}

void Service::connect()
{
    if (!mStepSensor.isConnected()) {
        mStepSensor.connect();
    }
}

void Service::disconnect()
{
    if (mStepSensor.isConnected()) {
        mStepSensor.disconnect();
    }
}

void Service::handleSensorData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    if (!mStepSensor.matchesDriver(handle)) {
        return;
    }

    // The batch is built here out of fields the event arrived with, and
    // DataBatch guards its index with an assert, which a release build drops.
    // So the size is checked rather than assumed, and the newest sample is the
    // one read: a batch holds its samples oldest first, and only the last of
    // them is the count as it stands now.
    if (data.size() == 0) {
        return;
    }

    SDK::SensorDataParser::StepCounter parser(data[data.size() - 1]);
    if (!parser.isDataValid()) {
        return;
    }

    mSteps = parser.getStepCount();
    publishSteps();
}

void Service::republishAll()
{
    mTimeSent   = false;
    mStepsSent  = false;
    mFormatSent = false;

    // The clock is republished by the loop's next turn, which is immediate.
    publishSteps();
    refreshSystemSettings();
}

void Service::refreshSystemSettings()
{
    // Stamped whether or not the read succeeds, so a kernel that is not
    // answering is retried on the next poll rather than on every turn.
    mSettingsAt = mKernel.sys.getTimeMs();

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(kSettingsTimeoutMs) && msg.ok()) {
            mIs12h      = msg->timeFormat;
            mMonthFirst = msg->dateMonthFirst;
        }
    }

    // Published unconditionally rather than only on a successful read: the
    // publisher drops an unchanged value anyway, and a failed read leaves
    // mIs12h at whatever was last known, which is still the best answer there
    // is. What this does guarantee is that the GUI hears a format at least
    // once, even if the very first request times out.
    publishClockFormat();
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

void Service::publishSteps()
{
    if (mStepsSent && (mSteps == mSentSteps)) {
        return;
    }

    mSentSteps = mSteps;
    mStepsSent = SDK::send_msg<CustomMessage::Steps>(mKernel, mSentSteps);
}

void Service::publishClockFormat()
{
    if (mFormatSent && (mIs12h == mSentIs12h) &&
            (mMonthFirst == mSentMonthFirst)) {
        return;
    }

    mSentIs12h      = mIs12h;
    mSentMonthFirst = mMonthFirst;
    mFormatSent     = SDK::send_msg<CustomMessage::ClockFormat>(
        mKernel, mSentIs12h, mSentMonthFirst);
}
