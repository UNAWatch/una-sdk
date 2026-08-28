#include "Service.hpp"
#include "Commands.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"

#include <ctime>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

static constexpr uint32_t kSecondsPerMinute = 60;
static constexpr uint32_t kMsPerSecond      = 1000;

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
    , mBatterySensor(SDK::Sensor::Type::BATTERY_LEVEL)
    , mHour(0)
    , mMinute(0)
    , mMday(0)
    , mWday(0)
    , mTimeSent(false)
    , mLevel(0)
    , mLevelSent(false)
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
    // moment worth deferring the subscription to. Connecting publishes the
    // level once straight away, which is what fills the indicator on boot.
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

        SDK::MessageBase *msg;
        if (!mKernel.comm.getMessage(msg, msToNextMinute(local))) {
            continue;
        }

        bool done = false;

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto *event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                handleSensorData(event->handle, batch);
            } break;

            // Nothing to hand over -- the GUI reads the clock itself when its
            // Model is built, and the level was published on connect. The line
            // is the marker the simulator smoke test looks for.
            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
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
    if (!mBatterySensor.isConnected()) {
        mBatterySensor.connect();
    }
}

void Service::disconnect()
{
    if (mBatterySensor.isConnected()) {
        mBatterySensor.disconnect();
    }
}

void Service::handleSensorData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    if (!mBatterySensor.matchesDriver(handle)) {
        return;
    }

    // The batch is built here out of fields the event arrived with, and
    // DataBatch guards its index with an assert, which a release build drops.
    // So the size is checked rather than assumed, and the newest sample is the
    // one read: a batch holds its samples oldest first, and only the last of
    // them is the level as it stands now.
    if (data.size() == 0) {
        return;
    }

    SDK::SensorDataParser::BatteryLevel parser(data[data.size() - 1]);
    if (!parser.isDataValid()) {
        return;
    }

    publishBatteryLevel(static_cast<uint8_t>(parser.getCharge()));
}

void Service::publishTime(const std::tm &local)
{
    const uint8_t hour   = static_cast<uint8_t>(local.tm_hour);
    const uint8_t minute = static_cast<uint8_t>(local.tm_min);
    const uint8_t mday   = static_cast<uint8_t>(local.tm_mday);
    const uint8_t wday   = static_cast<uint8_t>(local.tm_wday);

    if (mTimeSent && (hour == mHour) && (minute == mMinute) &&
        (mday == mMday) && (wday == mWday)) {
        return;
    }

    mHour     = hour;
    mMinute   = minute;
    mMday     = mday;
    mWday     = wday;
    mTimeSent = true;

    SDK::send_msg<CustomMessage::Time>(mKernel, hour, minute, mday, wday);
}

void Service::publishBatteryLevel(uint8_t level)
{
    // The indicator resolves to 25 % bands, so a fractional move is not worth
    // an IPC round trip; whole percent is already finer than it can show.
    if (mLevelSent && (level == mLevel)) {
        return;
    }

    mLevel     = level;
    mLevelSent = true;

    SDK::send_msg<CustomMessage::Battery>(mKernel, mLevel);
}

void Service::publishAlertsMuted(bool muted)
{
    if (mMutedSent && (muted == mMuted)) {
        return;
    }

    mMuted     = muted;
    mMutedSent = true;

    SDK::send_msg<CustomMessage::AlertsMuted>(mKernel, mMuted);
}
