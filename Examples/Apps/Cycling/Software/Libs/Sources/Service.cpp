
#include "Service.hpp"

#include <ctime>
#include <cmath>
#include <memory>
#include <cstring>

#include "Settings.hpp"
#include "ActivitySummary.hpp"
#include "Track.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/AccessoryMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Timer/Timer.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsSpeed.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsDistance.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserPressure.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace {

static float getPace(float speed, float threshold)
{
    return (speed > threshold) ? (1.0f / speed) : 0.0f;
}

// Average speed per the FIT definition: total distance / active timer time.
// This is NOT the mean of the instantaneous speed samples -- a sample mean
// drifts away from distance/time because sub-threshold samples are dropped
// from the average while their seconds still count toward the duration. Using
// the totals keeps distance, duration and the reported average pace mutually
// consistent on every lap and session.
static float speedFromTotals(float distanceM, float activeTimeS)
{
    return (activeTimeS > 0.0f) ? (distanceM / activeTimeS) : 0.0f;
}

} // namespace

Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mSettings{}
        , mSettingsSerializer(mKernel, "settings.json")
        , mSummary{}
        , mActivitySummarySerializer(mKernel, "Activity/summary.json")
        , mActivityWriter(mKernel, "Activity")
        , mTrackMapBuilder{}
        , mSensorGpsLocation(SDK::Sensor::Type::GPS_LOCATION, skSamplePeriod, skSampleLatency)
        , mSensorGpsSpeed(SDK::Sensor::Type::GPS_SPEED, skSamplePeriod, skSampleLatency)
        , mSensorGpsDistance(SDK::Sensor::Type::GPS_DISTANCE, skSamplePeriod, skSampleLatency)
        , mSensorPressure(SDK::Sensor::Type::PRESSURE, skSamplePeriod, skSampleLatency)
        , mSensorHr(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
        , mSensorBatteryLevel(SDK::Sensor::Type::BATTERY_LEVEL)
        , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, skSamplePeriod, skSampleLatency)
        , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION)
        , mTimeTracker(kernel.sys)
        , mAltitudeFilter(0.8f)
        , mAltitudeCounter()
        , mBatterySoc(kernel.sys)
        , mBatteryVoltage(kernel.sys)
{
    mTimeCounter.init();
    mDistanceCounter.init();
    mSpeedCounter.init(0.5f, 300.0f);
    mSpeedSmoother.init(0.5f, 300.0f);  // same valid range as the raw counter
    mHrCounter.init(20.0f, 300.0f);
    mAltitudeCounter.init(2.0f);
}

Service::~Service()
{
    disconnect();
}

void Service::run()
{
    LOG_INFO("Started\n");

    // Initialize time
    mTimeTracker.init();

    // Get settings
    if (!mSettingsSerializer.load(mSettings)) {
        LOG_WARNING("Failed to load settings\n");
    }

    // Get summary
    if (!mActivitySummarySerializer.load(mSummary)) {
        LOG_WARNING("Failed to load activity summary\n");
    }

    // Recover any activity a previous boot left unfinished (power loss /
    // crash mid-recording), before any new track can start.
    if (mActivityWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }

    SDK::Timer guiInitTimeout(TIMER_SECONDS(5));
    guiInitTimeout.start();

    bool firstFix = false;
    std::time_t processedUtc = 0;

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 500)) {
            // Command handling
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    disconnect();   // Cleanup resources
                    if (mTrackState != Track::State::INACTIVE) {
                        stopTrack(false);
                    }
                    // We must release message because this is the last event.
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    onStartGUI();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    onStopGUI();
                    break;

                // Custom messages
                case CustomMessage::SETTINGS_SAVE: {
                    LOG_DEBUG("SETTINGS_SAVE\n");
                    handleEvent(*static_cast<CustomMessage::SettingsSave*>(msg));
                } break;

                case CustomMessage::TRACK_START: {
                    LOG_DEBUG("TRACK_START\n");
                    handleEvent(*static_cast<CustomMessage::TrackStart*>(msg));
                } break;

                case CustomMessage::TRACK_STOP: {
                    LOG_DEBUG("TRACK_STOP\n");
                    handleEvent(*static_cast<CustomMessage::TrackStop*>(msg));
                } break;

                case CustomMessage::TRACK_PAUSE: {
                    LOG_DEBUG("TRACK_PAUSE\n");
                    handleEvent(*static_cast<CustomMessage::TrackPause*>(msg));
                } break;

                case CustomMessage::TRACK_RESUME: {
                    LOG_DEBUG("TRACK_RESUME\n");
                    handleEvent(*static_cast<CustomMessage::TrackResume*>(msg));
                } break;

                case CustomMessage::MANUAL_LAP: {
                    LOG_DEBUG("MANUAL_LAP\n");
                    handleEvent(*static_cast<CustomMessage::ManualLap*>(msg));
                } break;

                // Sensors messages
                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                    handleSensorsData(event->handle, batch);
                } break;

                // External accessory link status (WP-S4) -> forward to GUI for
                // the pre-activity HR indicator.
                case SDK::MessageType::EVENT_ACCESSORY_STATUS: {
                    auto* evt = static_cast<SDK::Message::Accessory::EventStatus*>(msg);
                    LOG_INFO("Accessory status: state %u\n", evt->state);
                    SDK::send_msg<CustomMessage::AccessoryStatusUpd>(mKernel, evt->state, evt->name);
                } break;

                default:
                    break;
            }
            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        if (mGuiStarted) {
            // Update time every second
            std::time_t utc = mTimeTracker.getExpectedUTC();

            if (processedUtc != utc) {
                processedUtc = utc;

                // GPS_LOCATION is subscribed once at GUI start so acquisition
                // begins on the pre-activity screen. That first attempt can lose
                // a ~100 ms connect race during app startup, which would strand
                // position logging for the entire session (distance/speed still
                // record via the kernel's own GPS_LOCATION listener, so the run
                // looks complete but has no map). Retry until it takes.
                if (mGpsWanted && !mSensorGpsLocation.isConnected()) {
                    connectGps();
                    if (mGpsInitialConnectFailed && mSensorGpsLocation.isConnected()) {
                        mGpsInitialConnectFailed = false;
                        LOG_INFO("GPS location subscription recovered after a lost startup connect\n");
                    }
                }

                // Send to GUI real "local time" to display
                std::tm tmNow = mTimeTracker.getLocalTime(std::time(nullptr));
                SDK::send_msg<CustomMessage::Time>(mKernel, tmNow);

                SDK::send_msg<CustomMessage::Battery>(mKernel, static_cast<uint8_t>(mBatterySoc.get()));

                // Update GPS fix
                if (mPreviousGpsFixState != mGps.fix) {
                    mPreviousGpsFixState = mGps.fix;

                    if (!firstFix) {
                        notifyFirstFix();
                        firstFix = true;
                    }
                    SDK::send_msg<CustomMessage::GpsFix>(mKernel, mGps.fix);
                }

                if (mTrackState != Track::State::INACTIVE) {
                    mTimeCounter.add(utc);
                    processTrack();
                }
            }
        } else {
            // Just wait some time to see if GUI starts
            if (guiInitTimeout.expired()) {
                LOG_INFO("No activities, exiting service\n");
                return; // Exit app
            }
        }
    }
}

void Service::connectGps()
{
    if (!mSensorGpsLocation.isConnected()) {
        LOG_DEBUG("Connect to GPS sensor...\n");
        mSensorGpsLocation.connect();
    }
}

void Service::connectSensors()
{
    // Idempotent + self-healing: connect only sensors not already connected,
    // so a subscribe that lost the ~100 ms ack race at track start is retried
    // (pumped from processTrack each tick) instead of dropped for the whole
    // session. Already-connected sensors are skipped, so there is no churn.
    if (!mSensorBatteryLevel.isConnected())   { mSensorBatteryLevel.connect(); }
    if (!mSensorBatteryMetrics.isConnected()) { mSensorBatteryMetrics.connect(); }
    if (!mSensorGpsSpeed.isConnected())       { mSensorGpsSpeed.connect(); }
    if (!mSensorGpsDistance.isConnected())    { mSensorGpsDistance.connect(); }
    if (!mSensorPressure.isConnected())       { mSensorPressure.connect(); }
    if (!mSensorHr.isConnected())             { mSensorHr.connect(); }

    mIsSensorsConnected = true;
}

void Service::disconnect()
{
    if (mIsSensorsConnected) {
        LOG_DEBUG("Disconnect from sensors...\n");

        mSensorHr.disconnect();
        mSensorPressure.disconnect();
        mSensorGpsSpeed.disconnect();
        mSensorGpsDistance.disconnect();
        mSensorBatteryLevel.disconnect();
        mSensorBatteryMetrics.disconnect();

        mIsSensorsConnected = false;
    }

    // The activity is over (stopTrack) or the app is stopping: GPS is no longer
    // wanted, so the run() retry must not re-wake it. Release unconditionally --
    // disconnect() is a no-op if never subscribed, and firing it whenever a
    // handle is held is what releases a listener whose connect-ack timed out
    // (isConnected() would be false in exactly that case).
    mGpsWanted = false;
    LOG_DEBUG("Disconnect from GPS sensor...\n");
    mSensorGpsLocation.disconnect();
}

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorGpsLocation.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsLocation parser(data[0]);
        if (parser.isDataValid()) {
            mGps.timestamp = parser.getTimestamp();
            mGps.fix = parser.isCoordinatesValid();

            if (mGps.fix) { // Do not change position if no fix
                parser.getCoordinates(mGps.latitude, mGps.longitude, mGps.altitude);
            }
            LOG_DEBUG("Location: fix %u, lat %f, lon %f\n", mGps.fix, mGps.latitude, mGps.longitude);
        }
    } else if (mSensorGpsSpeed.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsSpeed parser(data[0]);
        if (parser.isDataValid()) {
            // These three latches also feed auto-pause (see updateAutoPause),
            // which needs the same isSpeedValid() gate: a dead-reckoned speed is
            // extrapolated, so in a tunnel it would otherwise fake the movement
            // that triggers an auto-resume.
            mGpsSpeedMs    = parser.getSpeed();  // raw instantaneous speed
            mGpsSpeedValid = parser.isSpeedValid();  // already excludes dead reckoning
            mGpsSpeedFresh = true;   // consumed by the pace smoother each tick
            // Only feed a current (valid-fix) speed into the aggregated metrics
            // so acquisition / fix-loss / dead-reckoning readings don't inflate
            // the max-speed statistics.
            if (mGpsSpeedValid) {
                mSpeedCounter.add(mGpsSpeedMs);
            }
            LOG_DEBUG("Speed:    %.2f m/s (valid %u)\n", mGpsSpeedMs, mGpsSpeedValid);
        }
    } else if (mSensorGpsDistance.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsDistance parser(data[0]);
        if (parser.isDataValid()) {
            mDistanceCounter.add(parser.getDistance());
            LOG_DEBUG("Distance: %.2f m\n", parser.getDistance());
        }
    } else if (mSensorPressure.matchesDriver(handle)) {
        SDK::SensorDataParser::Pressure parser(data[0]);
        if (parser.isDataValid()) {
            if (!mAltitudeCounter.isValid()) {
                mSeaLevelPressure = parser.getP0();
            }
            float altitude = parser.getAltitude(parser.getPressure(), mSeaLevelPressure);
            float filtered = mAltitudeFilter.execute(altitude);
            mAltitudeCounter.add(filtered);
            LOG_DEBUG("Altitude %.2f (Filtered %.2f) (P0 %f, Pa %f)\n", altitude, filtered, mSeaLevelPressure, parser.getPressure());
        }
    } else if (mSensorHr.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateEx parser(data[0]);
        if (parser.isDataValid()) {
            mHrCounter.add(parser.getBpm());           // arbitrated (kernel's choice)
            mTrackData.hrTrustLevel = parser.getTrustLevel();
            mHrSource     = static_cast<uint8_t>(parser.getSource());
            mHrOpticalBpm = static_cast<uint8_t>(parser.getOpticalBpm());
            mHrExternalBpm= static_cast<uint8_t>(parser.getExternalBpm());
            LOG_DEBUG("HR %.1f trust %.1f src %u (opt %u ext %u)\n",
                      parser.getBpm(), parser.getTrustLevel(), mHrSource,
                      mHrOpticalBpm, mHrExternalBpm);
        }
    } else if (mSensorBatteryLevel.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel parser(data[0]);
        if (parser.isDataValid()) {
            mBatterySoc.set(parser.getCharge());
            LOG_DEBUG("Battery %.1f %%\n", mBatterySoc.get());
        }
    } else if (mSensorBatteryMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryMetrics parser(data[0]);
        if (parser.isDataValid()) {
            mBatteryVoltage.set(parser.getVoltage());
            LOG_DEBUG("Battery voltage %.1f V\n", mBatteryVoltage.get());
        }
    } else if (mSensorWristMotion.matchesDriver(handle)) {
        SDK::SensorDataParser::WristMotion parser(data[0]);
        if (parser.isDataValid()) {
            LOG_DEBUG("Wrist Motion detected\n");
            backlightOn();
        }
    }
}

void Service::onStartGUI()
{
    mGuiStarted = true;

    setCapabilities();
    requestAccessoryPrepare();   // pre-warm external HR while on the pre-activity screen

    // GPS stays wanted from the pre-activity screen until the activity ends
    // (cleared in disconnect()), so the run() loop keeps it connected during the
    // activity but never re-wakes the GNSS on the post-activity summary screen.
    mGpsWanted = true;

    // Subscribe to GPS to get fix. If this first attempt loses the ~100 ms
    // startup ack race, the run() loop retries; track the outcome so the retry
    // logs the recovery (and field logs reveal how often the race fires).
    connectGps();
    mGpsInitialConnectFailed = !mSensorGpsLocation.isConnected();
    if (mGpsInitialConnectFailed) {
        LOG_WARNING("GPS location subscribe lost the startup race; will retry\n");
    }

    mSensorWristMotion.connect();

    sendInitialInfoToGui();
}

void Service::onStopGUI()
{
    mGuiStarted = false;

    requestAccessoryRelease();
    mSensorWristMotion.disconnect();
}

void Service::handleEvent(const CustomMessage::TrackStart& event)
{
    // We can synchronize the time because we haven't started the track yet,
    // and the GPS could have already updated the current time.
    mTimeTracker.init();
    startTrack(mTimeTracker.getExpectedUTC());
}

void Service::handleEvent(const CustomMessage::TrackStop& event)
{
    stopTrack(event.discard);
}

void Service::handleEvent(const CustomMessage::SettingsSave& event)
{
    bool updCaps = mSettings.phoneNotifEn != event.settings.phoneNotifEn;
    mSettings = event.settings;
    mSettingsSerializer.save(event.settings);

    if (updCaps) {
        setCapabilities();
    }
}

void Service::handleEvent(const CustomMessage::TrackPause& /*event*/)
{
    pauseTrack(true, PauseSource::MANUAL);
}

void Service::handleEvent(const CustomMessage::TrackResume& /*event*/)
{
    pauseTrack(false, PauseSource::MANUAL);
}

void Service::handleEvent(const CustomMessage::ManualLap& /*event*/)
{
    saveLap();
    SDK::send_msg<CustomMessage::LapEnded>(mKernel, mTrackData.lapNum);
    notifyLapEnd();
}

void Service::setCapabilities()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestSetCapabilities>();
    if (msg) {
        msg->enPhoneNotification = mSettings.phoneNotifEn;
        msg->enUsbChargingScreen = false;
        msg->enMusicControl = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryPrepare()
{
    // Pre-acquire an external HR strap at the pre-activity screen (sent at
    // onStartGUI). No-op kernel-side unless external HR is enabled in Settings.
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestPrepare>();
    if (msg) {
        msg->kinds = SDK::Accessory::Kind::HRM;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryRelease()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestRelease>();
    if (msg) {
        msg->kinds = 0;   // release everything we acquired
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::notifyFirstFix()
{
    backlightOn();
    playBuzzerPattern(150, 3);
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::STRONG_CLICK_100);
}

void Service::notifyLapEnd()
{
    backlightOn();
    playBuzzerPattern(150, 3);
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100);
}

void Service::notifyAutoPause(bool paused)
{
    // Vibro only, and no forced backlight. Unlike a first fix or a lap end,
    // auto-pause fires at every traffic light, so a buzzer or a 5 s backlight
    // pulse each time would be both irritating and a needless battery cost.
    // The on-screen paused indicator carries the state; this is just the cue to
    // look. One pulse on pause, two on resume, so they are distinguishable
    // without looking.
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::STRONG_CLICK_100,
                     paused ? 1 : 2);
}

void Service::notifyNewActivity()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::CommandAppNewActivity>();
    if (msg) {
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::backlightOn(uint32_t timeoutMs)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}

void Service::playBuzzerPattern(uint16_t beepMs, uint8_t count, uint16_t silenceMs)
{
    if (count == 0) {
        return;
    }

    const uint8_t maxCount = (SDK::Message::RequestBuzzerPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
    if (msg) {
        uint8_t n = 0;
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[n].volume = 100;
            msg->notes[n].time = beepMs;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].volume = 0;
                msg->notes[n].time = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count, uint16_t silenceMs)
{
    if (count == 0) {
        return;
    }

    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (msg) {
        uint8_t n = 0;
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[n].effect = static_cast<uint8_t>(effect);
            msg->notes[n].pause = 0;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].effect = static_cast<uint8_t>(SDK::Message::RequestVibroPlay::Effect::NO_EFFECT);
                msg->notes[n].pause = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

ActivityWriter::RecordData Service::prepareRecordData()
{
    ActivityWriter::RecordData fitRecord{};

    fitRecord.timestamp = mTimeCounter.getCurrent();

    fitRecord.set(ActivityWriter::RecordData::Field::COORDS, mGps.fix);
    fitRecord.latitude  = mGps.latitude;
    fitRecord.longitude = mGps.longitude;

    fitRecord.set(ActivityWriter::RecordData::Field::SPEED, mSpeedCounter.isValid());
    fitRecord.speed = mSpeedCounter.getCurrent();

    fitRecord.set(ActivityWriter::RecordData::Field::ALTITUDE, mAltitudeCounter.isValid());
    fitRecord.altitude = mAltitudeCounter.getCurrent();

    bool hasHeartRate = (mHrCounter.getCurrent() > 20 && mTrackData.hrTrustLevel >= 1 && mTrackData.hrTrustLevel <= 3);
    fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
    fitRecord.heartRate = mHrCounter.getCurrent();
    // Tag each record with where the HR came from (none when no valid HR).
    fitRecord.hrSource     = hasHeartRate ? mHrSource : 0;
    fitRecord.hrOpticalBpm = mHrOpticalBpm;
    fitRecord.hrExternalBpm= mHrExternalBpm;

    // Both samples must be checked every call; evaluate separately to avoid short-circuit.
    const bool socReady     = mBatterySoc.isDue();
    const bool voltReady    = mBatteryVoltage.isDue();
    const bool batteryReady = socReady && voltReady;
    if (batteryReady) {
        mBatterySoc.consume();
        mBatteryVoltage.consume();
    }
    fitRecord.set(ActivityWriter::RecordData::Field::BATTERY, batteryReady);
    fitRecord.batteryLevel   = static_cast<uint8_t>(mBatterySoc.get());
    fitRecord.batteryVoltage = static_cast<uint16_t>(mBatteryVoltage.get() * 1000);

    return fitRecord;
}

void Service::sendInitialInfoToGui()
{
    uint8_t hrThresholds[CustomMessage::kHrThresholdsCount];
    memcpy(hrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(hrThresholds));
    uint8_t hrThresholdsCount = CustomMessage::kHrThresholdsCount;

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(100) && msg.ok()) {
            mIsImperial = msg->imperialUnits;
            mTimeFormat12h = msg->timeFormat;

            if (msg->heartRateCount > CustomMessage::kHrThresholdsCount) {
                msg->heartRateCount = CustomMessage::kHrThresholdsCount;
            }

            if (msg->heartRateCount > 0) {
                // Copy received elements
                uint8_t i = 0;
                for (; i < msg->heartRateCount; ++i) {
                    hrThresholds[i] = msg->heartRateTh[i];
                }

                // Complete the array elements to the full number
                for (; i < CustomMessage::kHrThresholdsCount; ++i) {
                    if (i > 0) {
                        hrThresholds[i] = hrThresholds[i - 1] + 20;
                    } else {
                        hrThresholds[i] = CustomMessage::kHrThresholdsDefault[0];
                    }
                }
            }
        }
    }

    SDK::send_msg<CustomMessage::SettingsUpd>(mKernel, mSettings, mIsImperial, mTimeFormat12h, hrThresholds, hrThresholdsCount);
    SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);
    SDK::send_msg<CustomMessage::Battery>(mKernel, static_cast<uint8_t>(mBatterySoc.get()));
}

void Service::startTrack(std::time_t utc)
{
    // Reset data
    mTrackData = {};

    mTimeCounter.reset();
    mTimeCounter.add(utc);

    mDistanceCounter.reset();
    mSpeedCounter.reset();
    mSpeedSmoother.reset();
    mGpsSpeedValid = false;
    mGpsSpeedFresh = false;
    mHrCounter.reset();
    mHrSource = 0;  // don't carry a prior track's HR source/readings into the new session
    mHrOpticalBpm = 0;
    mHrExternalBpm = 0;
    mAltitudeFilter.reset();
    mAltitudeCounter.reset();
    mBatterySoc.reset(skBatteryLogPeriodMs);
    mBatteryVoltage.reset(skBatteryLogPeriodMs);
    mGps.reset();

    mSessionNotEmpty = false;
    mLapNotEmpty = false;

    mPauseSource = PauseSource::NONE;
    mAutoPause.reset();

    mSummary = ActivitySummary{};
    mSummary.laps.reserve(10);

    // Configure TrackMapBuilder
    mTrackMapBuilder.reset();
    SDK::TrackMapBuilder::GpsPoint startGpsPoint{ mGps.latitude, mGps.longitude };
    mTrackMapBuilder.setDistanceThreshold(startGpsPoint, skMapDistanceThreshold);

    // Determine lap split source
    mLapDivSource = getLapDivSource();

    connectSensors();

    ActivityWriter::AppInfo info{};
    info.timestamp  = utc;
    info.appVersion = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID      = DEV_ID;
    info.appID      = APP_ID;
    mActivityWriter.start(info);

    mTrackState = Track::State::ACTIVE;

    LOG_INFO("Track started. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
    SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);
}

void Service::processTrack()
{
    LOG_DEBUG("Time: %u / %u\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));

    // Retry any track-sensor subscription that lost the connect-ack race at
    // track start. connectSensors() is idempotent, so this is a cheap no-op
    // once everything is connected, and it runs only while the track is active
    // (processTrack) so it never re-powers sensors after the track ends.
    connectSensors();

    // Evaluate auto-pause before anything else this tick, so that a transition
    // takes effect on the map point, the FIT record and the auto-lap tests
    // below rather than one second late.
    updateAutoPause();

    // Creating map
    SDK::TrackMapBuilder::GpsPoint newPoint{ mGps.latitude, mGps.longitude };
    if (mGps.fix && mTrackState == Track::State::ACTIVE) {
        mTrackMapBuilder.addPoint(newPoint);
    }

    // Time, s
    mTrackData.totalTime = mTimeCounter.getValueActive();
    mTrackData.lapTime   = mTimeCounter.getLapValueActive();

    // Distance, m
    mTrackData.distance    = mDistanceCounter.getValueActive();
    mTrackData.lapDistance = mDistanceCounter.getLapValueActive();

    // Speed, m/s
    //
    // The live speed and pace shown to the user are the rolling-window mean of
    // the GPS speed, not the latest sample: a single sample's noise moves the
    // readout far more than any real change of effort does. Fed here rather than
    // from the GPS_SPEED callback so the window advances once per track tick even
    // when a sample is missing, which ages a lost fix out of the window instead
    // of holding it forward. Advanced only while ACTIVE, so the readout freezes
    // for the duration of a pause. That is a deliberate change:
    // VariableCounter::add() latches its current value before its own pause
    // check, so the old readout went on tracking the raw speed of a standing
    // rider while the activity was paused.
    const bool moving = (mTrackState == Track::State::ACTIVE);
    if (moving) {
        mSpeedSmoother.tick(mGpsSpeedMs, mGpsSpeedValid && mGpsSpeedFresh);
    }
    // Consumed every tick, NOT only while active: updateAutoPause() reads this
    // latch to tell a tick that brought a sample from one that did not, and it
    // has to keep doing so while paused in order to notice the rider moving off
    // again. Left set through a pause it would pin the detector to "always
    // fresh", so a fix lost while stopped would auto-resume off a frozen speed.
    mGpsSpeedFresh = false;

    // A paused rider is, by definition, not moving. The smoother deliberately
    // freezes while paused (see above), which was invisible when a pause always
    // meant the action overlay was up -- but auto-pause leaves the track face on
    // screen, where a frozen "20.0 km/h" next to the paused banner reads as a
    // bug. Report zero instead; the averages and maxima are untouched.
    mTrackData.speed       = moving ? mSpeedSmoother.getSpeed() : 0.0f;
    mTrackData.avgSpeed    = speedFromTotals(mTrackData.distance, mTrackData.totalTime);
    mTrackData.maxSpeed    = mSpeedCounter.getMaximum();
    mTrackData.avgLapSpeed = speedFromTotals(mTrackData.lapDistance, mTrackData.lapTime);
    mTrackData.maxLapSpeed = mSpeedCounter.getLapMaximum();

    // Pace, s/m
    const float kMinSpeed = mSpeedCounter.getMinValid();
    mTrackData.pace    = moving ? mSpeedSmoother.getPace() : 0.0f;  // 0 = "--" while paused
    mTrackData.lapPace = getPace(mTrackData.avgLapSpeed, kMinSpeed);

    // HR
    mTrackData.hr       = mHrCounter.getCurrent();
    mTrackData.hrSource = mHrSource;  // for the in-activity source-driven HR icon
    mTrackData.avgHR    = mHrCounter.getAverage();
    mTrackData.maxHR    = mHrCounter.getMaximum();
    mTrackData.avgLapHR = mHrCounter.getLapAverage();
    mTrackData.maxLapHR = mHrCounter.getLapMaximum();

    // Elevation, m
    mTrackData.elevation = mAltitudeCounter.getCurrent();

    // Update GUI
    SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);

    if (mTrackState == Track::State::ACTIVE) {
        // Save record to the FIT file
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);

        mSessionNotEmpty = true;    // Session has at least one record
        mLapNotEmpty = true;        // Lap has at least one record

        bool  switchLap        = false;
        float autoLapDistanceM = 0.0f;  // >0 marks a grid-aligned distance auto-lap
        switch (mLapDivSource) {
        case LapDivSource::DISTANCE: {
            const float target = Settings::Alerts::Distance::toMeters(mSettings.alertDistanceId, mIsImperial);
            if (mDistanceCounter.getLapValueActive() >= target) {
                switchLap        = true;
                autoLapDistanceM = target;
            }
            break;
        }
        case LapDivSource::TIME:
            switchLap = static_cast<uint32_t>(mTimeCounter.getLapValueActive()) >= Settings::Alerts::Time::toSeconds(mSettings.alertTimeId);
            break;
        case LapDivSource::OFF:
        default:
            break;
        }

        if (switchLap) {
            // A distance auto-lap is recorded at exactly the target distance,
            // with the overshoot carried into the next lap. Before saveLap()
            // (which increments lapNum and resets the lap counters), refresh the
            // lap fields the lap-alert popup reads so its speed is computed over
            // that same grid distance -- the live snapshot pushed earlier this
            // tick holds the small overshoot, whose speed would disagree with the
            // lap's whole-second duration. lapTime is still the completed lap's.
            if (autoLapDistanceM > 0.0f) {
                mTrackData.lapDistance = autoLapDistanceM;
                mTrackData.avgLapSpeed = speedFromTotals(autoLapDistanceM,
                                                         static_cast<float>(mTrackData.lapTime));
                mTrackData.lapPace     = getPace(mTrackData.avgLapSpeed, mSpeedCounter.getMinValid());
                SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);
            }

            saveLap(autoLapDistanceM);
            SDK::send_msg<CustomMessage::LapEnded>(mKernel, mTrackData.lapNum);
            notifyLapEnd();
        }
    }
}

void Service::saveLap(float autoLapDistanceM)
{
    const auto  lapTime     = mTimeCounter.getLapValueActive();
    // A distance auto-lap (autoLapDistanceM > 0) is recorded as exactly the
    // target distance; the overshoot is carried into the next lap below. This
    // keeps lap boundaries on the km/mi grid and makes the reported lap pace
    // agree with the lap duration.
    const bool  gridLap     = autoLapDistanceM > 0.0f;
    const float lapDistance = gridLap ? autoLapDistanceM
                                      : mDistanceCounter.getLapValueActive();
    const float lapSpeed    = speedFromTotals(lapDistance, static_cast<float>(lapTime));

    // Accumulate lap into summary
    mSummary.laps.push_back({
        lapTime,
        lapDistance,
        lapSpeed
    });

    // Save lap to the FIT file
    ActivityWriter::LapData fitLap{};

    fitLap.timestamp = mTimeCounter.getCurrent();
    fitLap.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getLapValueTotal();
    fitLap.duration  = lapTime;
    fitLap.elapsed   = mTimeCounter.getLapValueTotal();

    fitLap.distance  = lapDistance;

    fitLap.speedAvg  = lapSpeed;
    fitLap.speedMax  = mSpeedCounter.getLapMaximum();

    fitLap.hrAvg     = mHrCounter.getLapAverage();
    fitLap.hrMax     = mHrCounter.getLapMaximum();

    fitLap.ascent    = mAltitudeCounter.getLapAscent();
    fitLap.descent   = mAltitudeCounter.getLapDescent();

    mActivityWriter.addLap(fitLap);

    mTrackData.lapNum++;

    LOG_INFO("Lap_%u saved. UTC: %u\n", mTrackData.lapNum, static_cast<uint32_t>(mTimeCounter.getCurrent()));
    LOG_INFO("Time: %u / %u s\n", static_cast<uint32_t>(mTimeCounter.getLapValueActive()), static_cast<uint32_t>(mTimeCounter.getLapValueTotal()));
    LOG_INFO("Distance: %.3f m\n", lapDistance);
    LOG_INFO("Speed: %.3f / %.3f m/s\n", lapSpeed, mSpeedCounter.getLapMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getLapAverage(), mHrCounter.getLapMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getLapAscent(), mAltitudeCounter.getLapDescent());

    // Reset lap counters
    mTimeCounter.resetLap();
    if (gridLap) {
        mDistanceCounter.advanceLap(lapDistance);
    } else {
        mDistanceCounter.resetLap();
    }
    mSpeedCounter.resetLap();
    mHrCounter.resetLap();
    mAltitudeCounter.resetLap();

    // Clear track data
    mTrackData.lapTime      = 0;
    mTrackData.lapDistance  = 0.0f;
    mTrackData.maxLapSpeed  = 0.0f;
    mTrackData.avgLapSpeed  = 0.0f;
    mTrackData.avgLapHR     = 0.0f;
    mTrackData.maxLapHR     = 0.0f;

    mLapNotEmpty = false;
}

void Service::buildPartialSummary()
{
    mSummary.utc       = mTimeCounter.getCurrent();
    mSummary.time      = mTimeCounter.getValueActive();
    mSummary.distance  = mDistanceCounter.getValueActive();
    mSummary.speedAvg  = speedFromTotals(mSummary.distance, mSummary.time);
    mSummary.elevation = mAltitudeCounter.getAscent();
    mSummary.paceAvg   = getPace(mSummary.speedAvg, mSpeedCounter.getMinValid());
    mSummary.hrMax     = mHrCounter.getMaximum();
    mSummary.hrAvg     = mHrCounter.getAverage();
    mSummary.map       = mTrackMapBuilder.build(skMapMaxPoints);
}

void Service::stopTrack(bool discard)
{
    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (!discard && mSessionNotEmpty) {

        if (mTrackState != Track::State::PAUSED) {
            // Ending the ride is always the rider's doing, so this closing stop
            // is Manual even when auto-pause was enabled for the session. Passed
            // explicitly rather than left to the default: this is the one
            // pause() caller that does not carry a PauseSource of its own.
            mActivityWriter.pause(mTimeCounter.getCurrent(), /*autoTrigger=*/false);
        }

        if (mLapNotEmpty) {
            saveLap();
        }

        mBatterySoc.request();
        mBatteryVoltage.request();
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);

        buildPartialSummary();

        // Save summary
        if (!mActivitySummarySerializer.save(mSummary)) {
            LOG_ERROR("Can't save activity summary\n");
        }
        SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);

        // Save FIT file
        ActivityWriter::TrackData fitTrack{};

        fitTrack.timestamp = mTimeCounter.getCurrent();
        fitTrack.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getValueTotal();
        fitTrack.duration  = mTimeCounter.getValueActive();
        fitTrack.elapsed   = mTimeCounter.getValueTotal();

        fitTrack.distance  = mDistanceCounter.getValueActive();

        fitTrack.speedAvg  = speedFromTotals(fitTrack.distance, fitTrack.duration);
        fitTrack.speedMax  = mSpeedCounter.getMaximum();

        fitTrack.hrAvg     = mHrCounter.getAverage();
        fitTrack.hrMax     = mHrCounter.getMaximum();

        fitTrack.ascent    = mAltitudeCounter.getAscent();
        fitTrack.descent   = mAltitudeCounter.getDescent();

        if (mActivityWriter.stop(fitTrack)) {
            notifyNewActivity();
        } else {
            LOG_ERROR("activity save failed\n");
            // Do NOT notify: the .fit is left unfinished, so the crash-recovery
            // marker (if any) stays for the next boot to finalize.
        }
    } else {
        mActivityWriter.discard();
    }

    mTrackState  = Track::State::INACTIVE;
    mPauseSource = PauseSource::NONE;
    mAutoPause.reset();
    LOG_INFO("Track stopped. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
    LOG_INFO("Time: %u / %u s\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));
    LOG_INFO("Distance: %.3f m\n", mDistanceCounter.getValueActive());
    LOG_INFO("Speed: %.3f / %.3f m/s\n", speedFromTotals(mDistanceCounter.getValueActive(), mTimeCounter.getValueActive()), mSpeedCounter.getMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getAverage(), mHrCounter.getMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getAscent(), mAltitudeCounter.getDescent());

    SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);

    disconnect();
}

void Service::pauseTrack(bool pause, PauseSource source)
{
    // Any pause/resume request invalidates the dwell evidence gathered for the
    // opposite edge, including the requests rejected by the guards below: a
    // blocked auto-resume must not leave its evidence banked for later. Done
    // first so no early return can skip it.
    mAutoPause.resetDwell();

    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (pause) {
        if (mTrackState == Track::State::ACTIVE) {
            mTimeCounter.pause();
            mDistanceCounter.pause();
            mSpeedCounter.pause();
            mHrCounter.pause();
            mAltitudeCounter.pause();

            // Pausing the counters also freezes the auto-lap sources: the lap
            // distance and lap time that processTrack() tests against the alert
            // targets are the *active* values, so a long wait at a light can no
            // longer trip a lap on its own.
            mActivityWriter.pause(mTimeCounter.getCurrent(),
                                  source == PauseSource::AUTO);

            mTrackState  = Track::State::PAUSED;
            mPauseSource = source;
            LOG_INFO("Track paused (%s). UTC: %u\n",
                     source == PauseSource::AUTO ? "auto" : "manual",
                     static_cast<uint32_t>(mTimeCounter.getCurrent()));
            SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);
        } else if (source == PauseSource::MANUAL && mPauseSource == PauseSource::AUTO) {
            // The rider opened the pause overlay while auto-pause was holding
            // the track. Hand ownership to them, so moving off again cannot
            // auto-resume recording underneath the save/discard menu.
            mPauseSource = PauseSource::MANUAL;
            LOG_INFO("Auto-pause taken over manually. UTC: %u\n",
                     static_cast<uint32_t>(mTimeCounter.getCurrent()));
        }

        // Only the manual path can reach a screen that shows the summary, and
        // nothing accumulates while paused, so building it on manual requests
        // alone keeps it fresh wherever it is visible without paying for it at
        // every traffic light.
        if (source == PauseSource::MANUAL) {
            buildPartialSummary();
            SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);
        }
    } else {
        if (mTrackState != Track::State::PAUSED) {
            return;
        }

        // An auto-resume must never lift a pause the rider asked for.
        if (source == PauseSource::AUTO && mPauseSource != PauseSource::AUTO) {
            return;
        }

        mTimeCounter.resume();
        mDistanceCounter.resume();
        mSpeedCounter.resume();
        // Drop the pre-pause window: those samples describe the effort before
        // the break, so blending them into the resumed readout would be wrong.
        mSpeedSmoother.reset();
        mHrCounter.resume();
        mAltitudeCounter.resume();

        mActivityWriter.resume(mTimeCounter.getCurrent(),
                               source == PauseSource::AUTO);

        mTrackState  = Track::State::ACTIVE;
        mPauseSource = PauseSource::NONE;
        LOG_INFO("Track resumed (%s). UTC: %u\n",
                 source == PauseSource::AUTO ? "auto" : "manual",
                 static_cast<uint32_t>(mTimeCounter.getCurrent()));
        SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);
    }
}

void Service::updateAutoPause()
{
    if (!mSettings.autoPauseEn) {
        // Switched off mid-activity while it was holding the track: give the
        // rider back an active track rather than stranding them paused.
        if (mTrackState == Track::State::PAUSED && mPauseSource == PauseSource::AUTO) {
            LOG_INFO("Auto-pause disabled while paused -- resuming\n");
            // MANUAL: the rider caused this by turning the setting off, so the
            // FIT records a rider-triggered resume. It also clears the AUTO
            // guard in pauseTrack() rather than depending on it.
            pauseTrack(false, PauseSource::MANUAL);
        }
        mAutoPause.reset();
        return;
    }

    // Freshness: this runs on the 1 Hz track tick and GPS speed arrives at the
    // same rate, but the two are not phase-locked, so a given tick may see zero
    // or two samples. Counting ticks-since-a-sample rather than demanding one
    // per tick tolerates that jitter.
    // Read the shared latches; do NOT clear mGpsSpeedFresh -- processTrack()
    // consumes it for the pace smoother after this runs.
    const bool fresh = mGpsSpeedFresh && mGpsSpeedValid;
    if (fresh) {
        mAutoPause.staleSec = 0;
    } else if (mAutoPause.staleSec < skAutoPauseStaleSec) {
        ++mAutoPause.staleSec;
    }

    // Age the braking reference on every tick, including the ones this function
    // returns early from, then DROP it once it is older than the cap.
    //
    // Clamping the age instead would be actively dangerous: the rate is
    // reference-minus-current over the age, so a reference eight ticks old
    // divided by a clamped three overstates the deceleration by 8/3. The first
    // sample after a long GPS gap -- riding out from under a bridge at 35 km/h,
    // where the reacquired Doppler sample is characteristically low -- would
    // then look like a hard stop and fire the fast path while the rider is at
    // speed. Dropping the reference makes the fast path simply not fire until a
    // fresh pair exists, which is what the stale-hold above is for.
    if (mAutoPause.prevAgeTicks < skAutoPauseDecelMaxAgeTicks) {
        ++mAutoPause.prevAgeTicks;
    } else {
        mAutoPause.prevValid = false;
    }

    // No trustworthy speed: hold whatever state we are in. Auto-pausing on a
    // lost fix would punish riding into a tunnel, and auto-resuming on one
    // would punish stopping in it.
    //
    // staleSec starts saturated (see AutoPauseState), so a track begun before
    // the first fix -- the rider hits Start in a car park and pedals off while
    // the GNSS is still acquiring -- holds here instead of acting on
    // mGpsSpeedMs, which at that point is an initialiser rather than a
    // measurement. Without that, the detector would read a never-measured
    // 0 m/s and auto-pause 3 s into every such ride.
    if (mAutoPause.staleSec >= skAutoPauseStaleSec) {
        mAutoPause.resetDwell();
        return;
    }

    // The dwell counts SAMPLES, not ticks. Advancing it on a tick that brought
    // no new sample would let one reading plus two dropouts satisfy a
    // three-sample threshold -- the inverse of the tolerance intended, and
    // reachable in the field because one obstruction (bridge, underpass) tends
    // to produce both the bad sample and the dropout that follows it. A gap
    // tick holds the evidence: it neither advances nor discards it.
    if (!fresh) {
        return;
    }

    // Deceleration since the last CHANGED sample, m/s^2, positive when slowing.
    // Repeated values are skipped rather than read as zero deceleration: the
    // receiver republishes its previous speed when it has no fresh RMC.
    float decelMps2 = 0.0f;
    if (mAutoPause.prevValid && mAutoPause.prevAgeTicks > 0) {
        decelMps2 = (mAutoPause.prevSpeedMps - mGpsSpeedMs)
                    / static_cast<float>(mAutoPause.prevAgeTicks);
    }
    if (!mAutoPause.prevValid || mGpsSpeedMs != mAutoPause.prevSpeedMps) {
        mAutoPause.prevSpeedMps = mGpsSpeedMs;
        mAutoPause.prevAgeTicks = 0;
        mAutoPause.prevValid    = true;
    }

    if (mTrackState == Track::State::ACTIVE && mPauseSource == PauseSource::NONE) {
        mAutoPause.aboveSec = 0;

        // Braking fast path. One sample that is slow AND shedding speed hard
        // ARMS it; the next sample still being slow CONFIRMS it.
        //
        // The arming sample on its own cannot tell a rider stopping from one
        // braking hard for a junction and rolling through -- the speed trace is
        // identical until they either put a foot down or accelerate away, and
        // the arming sample comes before that is knowable. Rolling through
        // recovers speed on the very next sample, which withholds the
        // confirmation. Measured on the reference ride, requiring it costs
        // 0.3 s of the mean gain and still pauses 3 s earlier than the dwell on
        // a stop from speed.
        const bool braking = (mGpsSpeedMs < skAutoPauseBrakingCeilMps)
                             && (decelMps2 >= skAutoPauseBrakingDecelMps2);
        const bool brakingConfirmed = mAutoPause.brakingArmed
                                      && (mGpsSpeedMs < skAutoPauseBrakingCeilMps);
        mAutoPause.brakingArmed = braking;

        bool pauseNow = false;
        if (mGpsSpeedMs < skAutoPauseSpeedMps) {
            pauseNow = (++mAutoPause.belowSec >= skAutoPauseDwellSec);
        } else {
            mAutoPause.belowSec = 0;
        }

        // Checked after the dwell, but NOT as an else-branch of the
        // sub-threshold test: the crispest stops land below the pause threshold
        // and satisfy the fast path on the same sample, and there is no reason
        // to trust the evidence less when the rider is slower.
        if (!pauseNow && brakingConfirmed) {
            LOG_INFO("Auto-pause: braking fast path (%.2f m/s, -%.2f m/s^2)\n",
                     mGpsSpeedMs, decelMps2);
            pauseNow = true;
        }

        if (pauseNow) {
            pauseTrack(true, PauseSource::AUTO);
            notifyAutoPause(true);
        }
    } else if (mTrackState == Track::State::PAUSED && mPauseSource == PauseSource::AUTO) {
        mAutoPause.belowSec = 0;
        if (mGpsSpeedMs > skAutoResumeSpeedMps) {
            if (++mAutoPause.aboveSec >= skAutoPauseDwellSec) {
                pauseTrack(false, PauseSource::AUTO);
                notifyAutoPause(false);
            }
        } else {
            mAutoPause.aboveSec = 0;
        }
    } else {
        // Manually paused: the detector stays quiet until the rider resumes.
        mAutoPause.resetDwell();
    }
}

Service::LapDivSource Service::getLapDivSource()
{
    if (mSettings.alertDistanceId != Settings::Alerts::Distance::ID_OFF) {
        return LapDivSource::DISTANCE;
    }
    if (mSettings.alertTimeId != Settings::Alerts::Time::ID_OFF) {
        return LapDivSource::TIME;
    }
    return LapDivSource::OFF;
}

