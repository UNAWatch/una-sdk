
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
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Timer/Timer.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsSpeed.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsDistance.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserPressure.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
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

} // namespace

Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mGuiSender(kernel)
        , mSettings{}
        , mSettingsSerializer(mKernel, "settings.json")
        , mSummary{}
        , mActivitySummarySerializer(mKernel, "Activity/summary.json")
        , mActivityWriter(mKernel, "Activity")
        , mTrackMapBuilder{}
        , mSensorGpsLocation(SDK::Sensor::Type::GPS_LOCATION, skSamplePeriod, skSampleLatency)
        , mSensorGpsSpeed(SDK::Sensor::Type::GPS_SPEED, skSamplePeriod, skSampleLatency)
        , mSensorGpsDistance(SDK::Sensor::Type::GPS_DISTANCE, skSamplePeriod, skSampleLatency)
        , mSensorStepCounter(SDK::Sensor::Type::STEP_COUNTER)
        , mSensorFloorCounter(SDK::Sensor::Type::FLOOR_COUNTER)
        , mSensorPressure(SDK::Sensor::Type::PRESSURE, skSamplePeriod, skSampleLatency)
        , mSensorHr(SDK::Sensor::Type::HEART_RATE, skSamplePeriod, skSampleLatency)
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
    mHrCounter.init(20.0f, 300.0f);
    mAltitudeCounter.init(2.0f);
    mStepCounter.init();
    mFloorCounter.init();
}

Service::~Service()
{
    disconnect();   // Cleanup resources
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

                // Send to GUI real "local time" to display
                std::tm tmNow = mTimeTracker.getLocalTime(std::time(nullptr));
                mGuiSender.time(tmNow);

                mGuiSender.battery(static_cast<uint8_t>(mBatterySoc.get()));

                // Update GPS fix
                if (mPreviousGpsFixState != mGps.fix) {
                    mPreviousGpsFixState = mGps.fix;

                    if (!firstFix) {
                        notifyFirstFix();
                        firstFix = true;
                    }
                    mGuiSender.fix(mGps.fix);
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
    if (!mIsSensorsConnected) {
        LOG_DEBUG("Connect to sensors...\n");

        mSensorBatteryLevel.connect();
        mSensorBatteryMetrics.connect();
        mSensorGpsSpeed.connect();
        mSensorGpsDistance.connect();
        mSensorStepCounter.connect();
        mSensorFloorCounter.connect();
        mSensorPressure.connect();
        mSensorHr.connect();

        mIsSensorsConnected = true;
    }
}

void Service::disconnect()
{
    if (mIsSensorsConnected) {
        LOG_DEBUG("Disconnect from sensors...\n");

        mSensorHr.disconnect();
        mSensorFloorCounter.disconnect();
        mSensorPressure.disconnect();
        mSensorStepCounter.disconnect();
        mSensorGpsSpeed.disconnect();
        mSensorGpsDistance.disconnect();
        mSensorBatteryLevel.disconnect();
        mSensorBatteryMetrics.disconnect();

        mIsSensorsConnected = false;
    }

    if (mSensorGpsLocation.isConnected()) {
        LOG_DEBUG("Disconnect from GPS sensor...\n");
        mSensorGpsLocation.disconnect();
    }
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
            mSpeedCounter.add(parser.getSpeed());
            LOG_DEBUG("Speed:    %.2f m/s\n", parser.getSpeed());
        }
    } else if (mSensorGpsDistance.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsDistance parser(data[0]);
        if (parser.isDataValid()) {
            mDistanceCounter.add(parser.getDistance());
            LOG_DEBUG("Distance: %.2f m\n", parser.getDistance());
        }
    } else if (mSensorStepCounter.matchesDriver(handle)) {
        SDK::SensorDataParser::StepCounter parser(data[0]);
        if (parser.isDataValid()) {
            mStepCounter.add(parser.getStepCount());
            LOG_DEBUG("Steps %u\n", parser.getStepCount());
        }
    } else if (mSensorFloorCounter.matchesDriver(handle)) {
        SDK::SensorDataParser::FloorCounter parser(data[0]);
        if (parser.isDataValid()) {
            uint32_t newValue = parser.getFloorsUp() + parser.getFloorsDown();
            mFloorCounter.add(newValue);
            LOG_DEBUG("Floors %u (up: %u, down: %u)\n", newValue, parser.getFloorsUp(), parser.getFloorsDown());
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
        SDK::SensorDataParser::HeartRate parser(data[0]);
        if (parser.isDataValid()) {
            mHrCounter.add(parser.getBpm());
            mTrackData.hrTrustLevel = parser.getTrustLevel();
            LOG_DEBUG("HR %.1f, TrustLevel %.1f\n", parser.getBpm(), parser.getTrustLevel());
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

    // Subscribe to GPS to get fix
    connectGps();

    mSensorWristMotion.connect();

    sendInitialInfoToGui();
}

void Service::onStopGUI()
{
    mGuiStarted = false;

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
    pauseTrack(true);
}

void Service::handleEvent(const CustomMessage::TrackResume& /*event*/)
{
    pauseTrack(false);
}

void Service::handleEvent(const CustomMessage::ManualLap& /*event*/)
{
    saveLap();
    mGuiSender.lapEnd(mTrackData.lapNum);
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

    mGuiSender.settingsUpd(mSettings, mIsImperial, hrThresholds, hrThresholdsCount);
    mGuiSender.summary(&mSummary);
    mGuiSender.battery(static_cast<uint8_t>(mBatterySoc.get()));
}

void Service::startTrack(std::time_t utc)
{
    // Reset data
    mTrackData = {};

    mTimeCounter.reset();
    mTimeCounter.add(utc);

    mDistanceCounter.reset();
    mSpeedCounter.reset();
    mHrCounter.reset();
    mAltitudeFilter.reset();
    mAltitudeCounter.reset();
    mStepCounter.reset();
    mFloorCounter.reset();
    mBatterySoc.reset(skBatteryLogPeriodMs);
    mBatteryVoltage.reset(skBatteryLogPeriodMs);
    mGps.reset();

    mSessionNotEmpty = false;
    mLapNotEmpty = false;

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
    mGuiSender.trackState(mTrackState);
}

void Service::processTrack()
{
    LOG_DEBUG("Time: %u / %u\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));

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
    mTrackData.speed        = mSpeedCounter.getCurrent();
    mTrackData.avgSpeed     = mSpeedCounter.getAverage();
    mTrackData.maxSpeed     = mSpeedCounter.getMaximum();
    mTrackData.avgLapSpeed  = mSpeedCounter.getLapAverage();
    mTrackData.maxLapSpeed  = mSpeedCounter.getLapMaximum();

    // Pace, s/m
    const float kMinSpeed  = mSpeedCounter.getMinValid();
    mTrackData.pace        = getPace(mTrackData.speed, kMinSpeed);
    mTrackData.avgPace     = getPace(mTrackData.avgSpeed, kMinSpeed);
    mTrackData.lapPace     = getPace(mTrackData.avgLapSpeed, kMinSpeed);

    // HR
    mTrackData.hr       = mHrCounter.getCurrent();
    mTrackData.avgHR    = mHrCounter.getAverage();
    mTrackData.maxHR    = mHrCounter.getMaximum();
    mTrackData.avgLapHR = mHrCounter.getLapAverage();
    mTrackData.maxLapHR = mHrCounter.getLapMaximum();

    // Elevation, m
    mTrackData.elevation = mAltitudeCounter.getCurrent();
    mTrackData.ascent    = mAltitudeCounter.getAscent();
    mTrackData.lapAscent = mAltitudeCounter.getLapAscent();

    // Steps
    mTrackData.steps    = mStepCounter.getValueActive();
    mTrackData.lapSteps = mStepCounter.getLapValueActive();

    // Floors
    mTrackData.floors    = mFloorCounter.getValueActive();
    mTrackData.lapFloors = mFloorCounter.getLapValueActive();

    // Update GUI
    mGuiSender.trackData(mTrackData);

    if (mTrackState == Track::State::ACTIVE) {
        // Save record to the FIT file
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);

        mSessionNotEmpty = true;    // Session has at least one record
        mLapNotEmpty = true;        // Lap has at least one record

        bool switchLap = false;
        switch (mLapDivSource) {
        case LapDivSource::DISTANCE:
            switchLap = mDistanceCounter.getLapValueActive() >= Settings::Alerts::Distance::toMeters(mSettings.alertDistanceId, mIsImperial);
            break;
        case LapDivSource::TIME:
            switchLap = static_cast<uint32_t>(mTimeCounter.getLapValueActive()) >= Settings::Alerts::Time::toSeconds(mSettings.alertTimeId);
            break;
        case LapDivSource::OFF:
        default:
            break;
        }

        if (switchLap) {
            saveLap();
            mGuiSender.lapEnd(mTrackData.lapNum);
            notifyLapEnd();
        }
    }
}

void Service::saveLap()
{
    // Accumulate lap into summary
    mSummary.laps.push_back({
        mTimeCounter.getLapValueActive(),
        mDistanceCounter.getLapValueActive(),
        mStepCounter.getLapValueActive()
    });

    // Save lap to the FIT file
    ActivityWriter::LapData fitLap{};

    fitLap.timestamp = mTimeCounter.getCurrent();
    fitLap.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getLapValueTotal();
    fitLap.duration  = mTimeCounter.getLapValueActive();
    fitLap.elapsed   = mTimeCounter.getLapValueTotal();

    fitLap.distance  = mDistanceCounter.getLapValueActive();

    fitLap.speedAvg  = mSpeedCounter.getLapAverage();
    fitLap.speedMax  = mSpeedCounter.getLapMaximum();

    fitLap.steps     = mStepCounter.getLapValueActive();
    fitLap.floors    = mFloorCounter.getLapValueActive();

    fitLap.hrAvg     = mHrCounter.getLapAverage();
    fitLap.hrMax     = mHrCounter.getLapMaximum();

    fitLap.ascent    = mAltitudeCounter.getLapAscent();
    fitLap.descent   = mAltitudeCounter.getLapDescent();

    mActivityWriter.addLap(fitLap);

    LapSummary lapSummary{};
    lapSummary.duration = mTimeCounter.getLapValueActive();
    lapSummary.distance = mDistanceCounter.getLapValueActive();
    lapSummary.steps    = mStepCounter.getLapValueActive();
    mSummary.laps.push_back(lapSummary);

    mTrackData.lapNum++;

    LOG_INFO("Lap_%u saved. UTC: %u\n", mTrackData.lapNum, static_cast<uint32_t>(mTimeCounter.getCurrent()));
    LOG_INFO("Time: %u / %u s\n", static_cast<uint32_t>(mTimeCounter.getLapValueActive()), static_cast<uint32_t>(mTimeCounter.getLapValueTotal()));
    LOG_INFO("Distance: %.3f m\n", mDistanceCounter.getLapValueActive());
    LOG_INFO("Speed: %.3f / %.3f m/s\n", mSpeedCounter.getLapAverage(), mSpeedCounter.getLapMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getLapAverage(), mHrCounter.getLapMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getLapAscent(), mAltitudeCounter.getLapDescent());
    LOG_INFO("Steps: %u\n", mStepCounter.getLapValueActive());
    LOG_INFO("Floors: %u\n", mFloorCounter.getLapValueActive());

    // Reset lap counters
    mTimeCounter.resetLap();
    mDistanceCounter.resetLap();
    mSpeedCounter.resetLap();
    mHrCounter.resetLap();
    mAltitudeCounter.resetLap();
    mStepCounter.resetLap();
    mFloorCounter.resetLap();

    // Clear track data
    mTrackData.lapTime     = 0;
    mTrackData.lapDistance = 0.0f;
    mTrackData.lapSteps    = 0;
    mTrackData.lapFloors   = 0;
    mTrackData.lapAscent   = 0.0f;
    mTrackData.maxLapSpeed = 0.0f;
    mTrackData.avgLapSpeed = 0.0f;
    mTrackData.avgLapHR    = 0.0f;
    mTrackData.maxLapHR    = 0.0f;

    mLapNotEmpty = false;
}

void Service::buildPartialSummary()
{
    mSummary.utc      = mTimeCounter.getCurrent();
    mSummary.time     = mTimeCounter.getValueActive();
    mSummary.distance = mDistanceCounter.getValueActive();
    mSummary.speedAvg = mSpeedCounter.getAverage();
    mSummary.steps    = mStepCounter.getValueActive();
    mSummary.elevation = mAltitudeCounter.getCurrent();
    mSummary.paceAvg  = getPace(mSpeedCounter.getAverage(), mSpeedCounter.getMinValid());
    mSummary.hrMax    = mHrCounter.getMaximum();
    mSummary.hrAvg    = mHrCounter.getAverage();
    mSummary.map      = mTrackMapBuilder.build(skMapMaxPoints);
}

void Service::stopTrack(bool discard)
{
    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (!discard && mSessionNotEmpty) {

        if (mTrackState != Track::State::PAUSED) {
            mActivityWriter.pause(mTimeCounter.getCurrent());
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
        mGuiSender.summary(&mSummary);

        // Save FIT file
        ActivityWriter::TrackData fitTrack{};

        fitTrack.timestamp = mTimeCounter.getCurrent();
        fitTrack.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getValueTotal();
        fitTrack.duration  = mTimeCounter.getValueActive();
        fitTrack.elapsed   = mTimeCounter.getValueTotal();

        fitTrack.distance  = mDistanceCounter.getValueActive();

        fitTrack.speedAvg  = mSpeedCounter.getAverage();
        fitTrack.speedMax  = mSpeedCounter.getMaximum();

        fitTrack.hrAvg     = mHrCounter.getAverage();
        fitTrack.hrMax     = mHrCounter.getMaximum();

        fitTrack.ascent    = mAltitudeCounter.getAscent();
        fitTrack.descent   = mAltitudeCounter.getDescent();

        fitTrack.steps     = mStepCounter.getValueActive();
        fitTrack.floors    = mFloorCounter.getValueActive();

        mActivityWriter.stop(fitTrack);

        notifyNewActivity();
    } else {
        mActivityWriter.discard();
    }

    mTrackState = Track::State::INACTIVE;
    LOG_INFO("Track stopped. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
    LOG_INFO("Time: %u / %u s\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));
    LOG_INFO("Distance: %.3f m\n", mDistanceCounter.getValueActive());
    LOG_INFO("Speed: %.3f / %.3f m/s\n", mSpeedCounter.getAverage(), mSpeedCounter.getMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getAverage(), mHrCounter.getMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getAscent(), mAltitudeCounter.getDescent());
    LOG_INFO("Steps: %u\n", mStepCounter.getValueActive());
    LOG_INFO("Floors: %u\n", mFloorCounter.getValueActive());

    mGuiSender.trackState(mTrackState);

    disconnect();
}

void Service::pauseTrack(bool pause)
{
    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (pause && mTrackState == Track::State::ACTIVE) {
        mTimeCounter.pause();
        mDistanceCounter.pause();
        mSpeedCounter.pause();
        mHrCounter.pause();
        mAltitudeCounter.pause();
        mStepCounter.pause();
        mFloorCounter.pause();

        mActivityWriter.pause(mTimeCounter.getCurrent());

        mTrackState = Track::State::PAUSED;
        LOG_INFO("Track paused. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
        mGuiSender.trackState(mTrackState);

        buildPartialSummary();
        mGuiSender.summary(&mSummary);
    } else if (!pause && mTrackState == Track::State::PAUSED) {
        mTimeCounter.resume();
        mDistanceCounter.resume();
        mSpeedCounter.resume();
        mHrCounter.resume();
        mAltitudeCounter.resume();
        mStepCounter.resume();
        mFloorCounter.resume();

        mActivityWriter.resume(mTimeCounter.getCurrent());

        mTrackState = Track::State::ACTIVE;
        LOG_INFO("Track resumed. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
        mGuiSender.trackState(mTrackState);
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

