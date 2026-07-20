
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
#include "SDK/Utils/Utils.hpp"
#include "SDK/Timer/Timer.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserPressure.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFusionRaw.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserRunningCadence.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace {

/** @brief Convert speed (m/s) to pace (s/m). Returns 0 if speed is below threshold. */
static float getPace(float speed, float threshold)
{
    return (speed > threshold) ? (1.0f / speed) : 0.0f;
}

// Average speed per the FIT definition: total distance / active timer time.
// This is NOT the mean of the instantaneous speed samples -- a sample mean
// drifts away from distance/time because sub-threshold samples are dropped
// from the average while their seconds still count toward the duration. Using
// the totals keeps distance, duration and the reported average pace mutually
// consistent on every lap and session. (finalizeActivity() already derives the
// session avg_speed this way; this matches the per-lap and live values to it.)
static float speedFromTotals(float distanceM, float activeTimeS)
{
    return (activeTimeS > 0.0f) ? (distanceM / activeTimeS) : 0.0f;
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
        , mSensorPressure(SDK::Sensor::Type::PRESSURE, skSamplePeriod, skSampleLatency)
        , mSensorHr(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
        , mSensorBatteryLevel(SDK::Sensor::Type::BATTERY_LEVEL)
        , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, skSamplePeriod, skSampleLatency)
        , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION)
        , mSensorFusion(SDK::Sensor::Type::FUSION_RAW, 1000.0f / skFusionSampleRateHz, 100)
        , mSensorRunningCadence(SDK::Sensor::Type::RUNNING_CADENCE, skSamplePeriod, skSampleLatency)
        , mSensorGrade(SDK::Sensor::Type::GRADE, skSamplePeriod, skSampleLatency)
        , mTimeTracker(kernel.sys)
        , mAltitudeFilter(0.8f)
        , mAltitudeCounter()
        , mBatterySoc(kernel.sys)
        , mBatteryVoltage(kernel.sys)
        , mWristTiltDetector()
        , mModel(mKernel.fs)
        , mEstimator(mModel)

{
    mTimeCounter.init();
    mDistanceCounter.init();
    mSpeedCounter.init(0.5f, 300.0f);
    mHrCounter.init(20.0f, 300.0f);
    mAltitudeCounter.init(2.0f);

    WristTiltDetector::Config config{};
    config.sampleRateHz = skFusionSampleRateHz;
    mWristTiltDetector.setConfig(config);

    mWristTiltDetector.setListener(this);
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

    // Recover any activity a previous boot left unfinished (power loss /
    // crash mid-recording), before any new track can start.
    if (mActivityWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }

    SDK::Timer guiInitTimeout(TIMER_SECONDS(5));
    guiInitTimeout.start();

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
                    // App is closing while a Calibrate & Save decision is pending:
                    // finalise with the estimated distance so the FIT file is closed.
                    if (mAwaitingCalibration) {
                        finalizeActivity(0.0f);
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
                case CustomMessage::SETTINGS_SAVE:  {
                    LOG_DEBUG("SETTINGS_SAVE\n");
                    handleEvent(*static_cast<CustomMessage::SettingsSave*>(msg));
                } break;

                case CustomMessage::TRACK_START:  {
                    LOG_DEBUG("TRACK_START\n");
                    handleEvent(*static_cast<CustomMessage::TrackStart*>(msg));
                } break;

                case CustomMessage::TRACK_STOP:  {
                    LOG_DEBUG("TRACK_STOP\n");
                    handleEvent(*static_cast<CustomMessage::TrackStop*>(msg));
                } break;

                case CustomMessage::TRACK_PAUSE:  {
                    LOG_DEBUG("TRACK_PAUSE\n");
                    handleEvent(*static_cast<CustomMessage::TrackPause*>(msg));
                } break;

                case CustomMessage::TRACK_RESUME:  {
                    LOG_DEBUG("TRACK_RESUME\n");
                    handleEvent(*static_cast<CustomMessage::TrackResume*>(msg));
                } break;

                case CustomMessage::MANUAL_LAP:  {
                    LOG_DEBUG("MANUAL_LAP\n");
                    handleEvent(*static_cast<CustomMessage::ManualLap*>(msg));
                } break;

                case CustomMessage::INTERVALS_NEXT_PHASE:  {
                    LOG_DEBUG("INTERVALS_NEXT_PHASE\n");
                    handleEvent(*static_cast<CustomMessage::IntervalsNextPhase*>(msg));
                } break;

                case CustomMessage::TRACK_CALIBRATE:  {
                    LOG_DEBUG("TRACK_CALIBRATE\n");
                    handleEvent(*static_cast<CustomMessage::TrackCalibrate*>(msg));
                } break;
                case CustomMessage::CALIB_DATA_REQUEST:  {
                    LOG_DEBUG("CALIB_DATA_REQUEST\n");
                    handleEvent(*static_cast<CustomMessage::CalibDataRequest*>(msg));
                } break;
                case CustomMessage::CALIB_CLEAR:  {
                    LOG_DEBUG("CALIB_CLEAR\n");
                    handleEvent(*static_cast<CustomMessage::CalibClear*>(msg));
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
                    mGuiSender.accessoryStatus(evt->state, evt->name);
                } break;

                default:
                    break;
            }
            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }


        // Periodic process
        if (mGuiStarted) {
            // Update time every second
            std::time_t utc = mTimeTracker.getExpectedUTC();

            if (processedUtc != utc) {
                processedUtc = utc;

                // Send to GUI real "local time" to display
                std::tm tmNow = mTimeTracker.getLocalTime(std::time(nullptr));
                mGuiSender.time(tmNow);

                mGuiSender.battery(static_cast<uint8_t>(mBatterySoc.get()));

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

        // Treadmill: no GPS (speed/distance), no grade, no barometer/altitude.
        // Speed and distance are derived from cadence by the estimator.
        mSensorBatteryLevel.connect();
        mSensorBatteryMetrics.connect();
        mSensorHr.connect();
        mSensorFusion.connect();
        mSensorRunningCadence.connect();

        mIsSensorsConnected = true;
    }
}

void Service::disconnect()
{
    if (mIsSensorsConnected) {
        LOG_DEBUG("Disconnect from sensors...\n");

        mSensorFusion.disconnect();
        mSensorRunningCadence.disconnect();
        mSensorHr.disconnect();
        mSensorBatteryLevel.disconnect();
        mSensorBatteryMetrics.disconnect();

        mIsSensorsConnected = false;
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
    } else if (mSensorRunningCadence.matchesDriver(handle)) {
        SDK::SensorDataParser::RunningCadence parser(data[0]);
        if (parser.isDataValid()) {
            mRunningCadence.cadenceSpm   = parser.getCadenceSpm();
            mRunningCadence.cadenceValid = parser.isCadenceValid();
        }
    } else if (mSensorFusion.matchesDriver(handle)) {
        static constexpr uint16_t kBatchSize = 10u;
        TiltImuSample batch[kBatchSize];
        uint16_t batchLen = 0;

        for (uint16_t i = 0; i < data.size(); i++) {
            SDK::SensorDataParser::FusionRaw parser(data[i]);
            if (parser.isDataValid()) {
                SDK::SensorDataParser::FusionRaw::Data sample{};
                parser.getData(sample);
                batch[batchLen].ayLsb = sample.accel.y;
                batch[batchLen].azLsb = sample.accel.z;
                batch[batchLen].gxLsb = sample.gyro.x;
                batch[batchLen].timestampMs = parser.getTimestamp();
                //LOG_DEBUG("AY: %d, GX: %d\n", sample.accel.y, sample.gyro.x);
                ++batchLen;

                if (batchLen == kBatchSize) {
                    mWristTiltDetector.addBatch(batch, kBatchSize);
                    batchLen = 0;
                }
            }
        }

        if (batchLen > 0u) {
            mWristTiltDetector.addBatch(batch, batchLen);
        }
    }
}


void Service::onStartGUI()
{
    mGuiStarted = true;

    setCapabilities();
    requestAccessoryPrepare();   // pre-warm external HR while on the pre-activity screen

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
    // If a previous activity is still awaiting a Calibrate & Save decision,
    // finalise it with the estimate before starting a new session.
    if (mAwaitingCalibration) {
        finalizeActivity(0.0f);
    }

    // We can synchronize the time because we haven't started the track yet.
    mTimeTracker.init();

    mIntervalsMode = event.intervalsMode;

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

    // A series of N beeps needs 2*N-1 notes (beeps + silences between them).
    // Cap to what fits in skMaxNotes: max count = (skMaxNotes + 1) / 2 = 5.
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

    // A series of N effects needs 2*N-1 notes (effects + silences between them).
    // Cap to what fits in skMaxNotes: max count = (skMaxNotes + 1) / 2 = 4.
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

    fitRecord.timestamp    = mTimeCounter.getCurrent();

    // No position/altitude on a treadmill (§7.4.1): speed is cadence-derived.
    fitRecord.set(ActivityWriter::RecordData::Field::SPEED, mSpeedCounter.isValid());
    fitRecord.speed        = mSpeedCounter.getCurrent();

    // Cumulative active distance (live uncorrected estimate, same source as the
    // session total pre-calibration). Monotonic and always valid while recording,
    // so it is written unconditionally.
    fitRecord.set(ActivityWriter::RecordData::Field::DISTANCE, true);
    fitRecord.distance     = mDistanceCounter.getValueActive();

    bool hasHeartRate = (mHrCounter.getCurrent() > 20 && mTrackData.hrTrustLevel >= 1 && mTrackData.hrTrustLevel <= 3);
    fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
    fitRecord.heartRate    = mHrCounter.getCurrent();
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

    // Record the same kernel cadence that drove the estimator this tick, so
    // cadence, step length and speed stay consistent.
    fitRecord.set(ActivityWriter::RecordData::Field::CADENCE, mRunningCadence.cadenceValid);
    fitRecord.cadenceSpm = mRunningCadence.cadenceSpm;

    // Treadmill step length comes from the cadence/stride model: single-step
    // distance = SL_model(c) / 2. Written only when cadence is valid (§7.4).
    const bool cadenceOk = mRunningCadence.cadenceValid;
    fitRecord.set(ActivityWriter::RecordData::Field::STEP_LENGTH, cadenceOk);
    fitRecord.stepLengthM = cadenceOk
        ? mModel.treadmillStrideLengthM(mRunningCadence.cadenceSpm) / 2.0f
        : 0.0f;

    return fitRecord;
}

void Service::sendInitialInfoToGui()
{
    // Settings
    uint8_t hrThresholds[CustomMessage::kHrThresholdsCount];
    memcpy(hrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(hrThresholds));

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(100) && msg.ok()) {
            mIsImperial = msg->imperialUnits;
            mTimeFormat12h = msg->timeFormat;

            // Height (cm -> m) drives the phase-1 demographic stride. The model
            // clamps implausible values at session start; only override the
            // default when the profile actually reports a height.
            if (msg->heightCm > 0) {
                mHeightM = static_cast<float>(msg->heightCm) / 100.0f;
            }

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

    mGuiSender.settingsUpd(mSettings, mIsImperial, mTimeFormat12h, hrThresholds, CustomMessage::kHrThresholdsCount);
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
    mHrSource = 0;  // don't carry a prior track's HR source/readings into the new session
    mHrOpticalBpm = 0;
    mHrExternalBpm = 0;
    mAltitudeFilter.reset();
    mAltitudeCounter.reset();
    mBatterySoc.reset(skBatteryLogPeriodMs);
    mBatteryVoltage.reset(skBatteryLogPeriodMs);
    mGps.reset();
    mRunningCadence = {};

    // Cadence/stride estimation: load the shared outdoor LUT + delta LUT and
    // freeze the calibration phase for the whole session, then zero the live
    // integrator. The model reads the LUTs read-only (the Running app owns the
    // outdoor LUT's integrity).
    mLastTickUtc = 0;
    mModel.startSession(mHeightM);
    mEstimator.startSession();

    mSessionNotEmpty = false;
    mLapNotEmpty = false;

    mSummary = ActivitySummary{};
    mSummary.laps.reserve(10);

    // Intervals mode initialization
    mIntervalsCompleted = false;
    mTrackData.intervalsMode = mIntervalsMode;
    if (mIntervalsMode) {
        const Settings::Intervals& cfg = mSettings.intervals;
        mTrackData.intervals = Track::IntervalsData{};
        // totalRepeats is the literal number of RUN-REST cycles the user selected.
        // repeatsNum == 0 means 'Open' (unlimited): totalRepeats stays 0 and the GUI
        // shows just the current repeat with no total.
        mTrackData.intervals.totalRepeats = cfg.repeatsNum;

        if (cfg.warmUp) {
            startIntervalsPhase(Track::IntervalsPhase::WARM_UP);
        } else {
            mTrackData.intervals.repeat = 1;
            startIntervalsPhase(Track::IntervalsPhase::RUN);
        }
    }

    // Configure TrackMapBuilder
    mTrackMapBuilder.reset();
    SDK::TrackMapBuilder::GpsPoint startGpsPoint{ mGps.latitude, mGps.longitude };
    mTrackMapBuilder.setDistanceThreshold(startGpsPoint, skMapDistanceThreshold);

    // Determine lap split source. In intervals mode each phase (warm up / run /
    // rest / cool down) is recorded as its own lap, so the alert-based auto-lap
    // is disabled to avoid splitting a phase across multiple laps.
    mLapDivSource = mIntervalsMode ? LapDivSource::OFF : getLapDivSource();

    mWristTiltDetector.reset();

    connectSensors();

    ActivityWriter::AppInfo info{};
    info.timestamp = utc;
    info.appVersion = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID = DEV_ID;
    info.appID = APP_ID;
    mActivityWriter.start(info);

    if (mIntervalsMode) {
        emitIntervalsWorkout();
    }

    mTrackState = Track::State::ACTIVE;

    mGuiSender.trackState(mTrackState);
}

void Service::processTrack()
{
    LOG_DEBUG("Time: %u / %u\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));

    // Cadence-driven speed/distance integration (replaces GPS). Runs only while
    // actively recording (pause halts it). Feeds the same counters that the live
    // metrics, laps, intervals and FIT records read, so the rest of the pipeline
    // is identical to Running.
    if (mTrackState == Track::State::ACTIVE) {
        const std::time_t nowUtc = mTimeCounter.getCurrent();
        const float dt = (mLastTickUtc == 0)
            ? 1.0f
            : static_cast<float>(nowUtc - mLastTickUtc);
        mLastTickUtc = nowUtc;

        // Kernel RUNNING_CADENCE is already a clean windowed rate - fed raw.
        mEstimator.tick(mRunningCadence.cadenceSpm, mRunningCadence.cadenceValid, dt);
        mDistanceCounter.add(mEstimator.distanceM());   // cumulative, monotonic
        // Feed speed every tick. speedMps() is 0 once the cadence hold-forward
        // window expires, so the current speed drops to 0 (display + FIT record)
        // instead of latching the last held value. add(0) is below minValid, so
        // it updates "current" only and never pollutes avg/max.
        mSpeedCounter.add(mEstimator.speedMps());
    }

    // Creating map
    SDK::TrackMapBuilder::GpsPoint newPoint{ mGps.latitude, mGps.longitude };
    // Add point to the track map
    if (mGps.fix && mTrackState == Track::State::ACTIVE) {
        mTrackMapBuilder.addPoint(newPoint);
    }

    // Time, s
    mTrackData.totalTime = mTimeCounter.getValueActive();
    mTrackData.lapTime = mTimeCounter.getLapValueActive();

    // Distance, m
    mTrackData.distance = mDistanceCounter.getValueActive();
    mTrackData.lapDistance = mDistanceCounter.getLapValueActive();

    // Speed, m/s
    mTrackData.speed = mSpeedCounter.getCurrent();

    mTrackData.avgSpeed    = speedFromTotals(mTrackData.distance, mTrackData.totalTime);
    mTrackData.maxSpeed    = mSpeedCounter.getMaximum();
    mTrackData.avgLapSpeed = speedFromTotals(mTrackData.lapDistance, mTrackData.lapTime);
    mTrackData.maxLapSpeed = mSpeedCounter.getLapMaximum();


    // Pace, s/m
    const float kMinSpeed = mSpeedCounter.getMinValid();
    mTrackData.pace = getPace(mTrackData.speed, kMinSpeed);
    mTrackData.avgPace = getPace(mTrackData.avgSpeed, kMinSpeed);
    mTrackData.lapPace = getPace(mTrackData.avgLapSpeed, kMinSpeed);


    // HR
    mTrackData.hr = mHrCounter.getCurrent();
    mTrackData.hrSource = mHrSource;  // for the in-activity source-driven HR icon
    mTrackData.avgHR = mHrCounter.getAverage();
    mTrackData.maxHR = mHrCounter.getMaximum();
    mTrackData.avgLapHR = mHrCounter.getLapAverage();
    mTrackData.maxLapHR = mHrCounter.getLapMaximum();


    // Altitude, m
    mTrackData.elevation = mAltitudeCounter.getCurrent();

    // Intervals state machine - only while actively running (not paused)
    if (mIntervalsMode && mTrackState == Track::State::ACTIVE) {
        processIntervals();
    }

    // Update GUI
    mGuiSender.trackData(mTrackData);


    if (mTrackState == Track::State::ACTIVE) {
        // Save record to the FIT file
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);

        mSessionNotEmpty = true;    // Session has at least one record
        mLapNotEmpty = true;        // Lap has at least one record

        // Next lap
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
                mGuiSender.trackData(mTrackData);
            }

            saveLap(autoLapDistanceM);
            mGuiSender.lapEnd(mTrackData.lapNum);
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
        getPace(lapSpeed, mSpeedCounter.getMinValid())
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

    // Link this lap to its workout_step (intervals only; left INVALID otherwise).
    if (mIntervalsMode && mIntervalsStepMap.valid) {
        fitLap.wktStepIndex = intervalsWktStepIndex();
    }

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
    mTrackData.lapTime = 0;
    mTrackData.lapDistance = 0.0f;
    mTrackData.maxLapSpeed = 0.0f;
    mTrackData.avgLapSpeed = 0.0f;
    mTrackData.avgLapHR = 0.0f;
    mTrackData.maxLapHR = 0.0f;

    mLapNotEmpty = false;
}

void Service::buildPartialSummary()
{
    mSummary.utc       = mTimeCounter.getCurrent();
    mSummary.time      = mTimeCounter.getValueActive();
    mSummary.distance  = mDistanceCounter.getValueActive();
    mSummary.speedAvg  = speedFromTotals(mSummary.distance, mSummary.time);
    mSummary.elevation = mAltitudeCounter.getCurrent();
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

    if (discard || !mSessionNotEmpty) {
        mActivityWriter.discard();
        mAwaitingCalibration = false;
        mTrackState = Track::State::INACTIVE;
        mGuiSender.trackState(mTrackState);
        disconnect();
        LOG_INFO("Track discarded.\n");
        return;
    }

    // Stop recording: flush the final lap + record and build the summary at the
    // ESTIMATED distance. FIT session finalisation is deferred until the
    // post-run Calibrate & Save decision (§5) so D_actual can be folded in.
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
    if (!mActivitySummarySerializer.save(mSummary)) {
        LOG_ERROR("Can't save activity summary\n");
    }
    mGuiSender.summary(&mSummary);

    // Snapshot the FIT session message and the inputs for the §5.3 delta update.
    mPendingFitTrack = ActivityWriter::TrackData{};
    mPendingFitTrack.timestamp = mTimeCounter.getCurrent();
    mPendingFitTrack.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getValueTotal();
    mPendingFitTrack.duration  = mTimeCounter.getValueActive();
    mPendingFitTrack.elapsed   = mTimeCounter.getValueTotal();
    mPendingFitTrack.distance  = mDistanceCounter.getValueActive();
    mPendingFitTrack.speedAvg  = mSpeedCounter.getAverage();
    mPendingFitTrack.speedMax  = mSpeedCounter.getMaximum();
    mPendingFitTrack.hrAvg     = mHrCounter.getAverage();
    mPendingFitTrack.hrMax     = mHrCounter.getMaximum();
    mPendingFitTrack.ascent    = mAltitudeCounter.getAscent();
    mPendingFitTrack.descent   = mAltitudeCounter.getDescent();

    {
        const float *hist = mEstimator.stepHistogram();
        for (size_t i = 0; i < SDK::Calibration::StrideLut::kBinCount; ++i) {
            mCalibSteps[i] = hist[i];
        }
    }
    mCalibDEstimatedM = mDistanceCounter.getValueActive();

    mTrackState = Track::State::INACTIVE;
    mGuiSender.trackState(mTrackState);
    disconnect();

    LOG_INFO("Track stopped. Time %u/%u s, Dist %.1f m\n",
             static_cast<uint32_t>(mTimeCounter.getValueActive()),
             static_cast<uint32_t>(mTimeCounter.getValueTotal()),
             static_cast<double>(mCalibDEstimatedM));

    // Every session waits for the GUI's Calibrate & Save decision so the user
    // can correct the recorded distance (and the avg speed derived from it) on
    // any run. The delta-LUT *learning* is gated separately inside
    // CadenceStrideModel::applyPostRunCalibration (outdoor-calibrated tier + a
    // minimum distance); correcting the FIT file is not.
    mAwaitingCalibration = true;
    LOG_INFO("Awaiting Calibrate & Save (D_est %.1f m)\n",
             static_cast<double>(mCalibDEstimatedM));
}

void Service::finalizeActivity(float distanceActualM)
{
    // distanceActualM <= 0 => skip calibration (keep the estimate). An accepted
    // value always rewrites the recorded distance; it additionally nudges the
    // delta LUT only in the outdoor-calibrated tier and above the minimum
    // distance (§5.3) — applyPostRunCalibration owns that gate.
    const SDK::Calibration::CadenceStrideModel::CalibrationResult res =
        mModel.applyPostRunCalibration(mCalibSteps, mCalibDEstimatedM, distanceActualM);

    // Keep the SESSION-level totals mutually consistent after the (possibly
    // corrected) distance is known. avg_speed is defined as
    // total_distance / total_timer_time, so derive it from the final distance --
    // distance, duration and pace then always agree, on every run. max_speed has
    // no "total" to derive from, so it takes the uniform stride-correction factor
    // k (= D_actual / D_estimated; 1.0 when calibration was skipped). Per-record
    // and per-lap data are intentionally left as recorded (industry convention).
    const float kCalib = (mCalibDEstimatedM > 1e-3f)
        ? (res.distanceForFitM / mCalibDEstimatedM)
        : 1.0f;

    mPendingFitTrack.distance = res.distanceForFitM;
    mPendingFitTrack.speedMax *= kCalib;
    mPendingFitTrack.speedAvg = (mPendingFitTrack.duration > 1e-3f)
        ? (res.distanceForFitM / mPendingFitTrack.duration)
        : 0.0f;
    // Always record the pre-correction estimate as a developer field (§5.4).
    mPendingFitTrack.distancePreCalibrationM = mCalibDEstimatedM;

    // Reflect the same derived totals in the saved + published summary.
    mSummary.distance = res.distanceForFitM;
    mSummary.speedAvg = (mSummary.time > 1e-3f)
        ? (res.distanceForFitM / mSummary.time)
        : 0.0f;
    mSummary.paceAvg = getPace(mSummary.speedAvg, mSpeedCounter.getMinValid());
    if (!mActivitySummarySerializer.save(mSummary)) {
        LOG_ERROR("Can't save activity summary\n");
    }
    mGuiSender.summary(&mSummary);

    if (mActivityWriter.stop(mPendingFitTrack)) {
        notifyNewActivity();
    } else {
        LOG_ERROR("activity save failed\n");
        // Do NOT notify: the .fit is left unfinished, so the crash-recovery
        // marker (if any) stays for the next boot to finalize.
    }

    mAwaitingCalibration = false;
    LOG_INFO("Activity finalised: dist %.1f m (calibrated=%d, deltaUpdated=%d)\n",
             static_cast<double>(res.distanceForFitM),
             res.dActualAccepted ? 1 : 0, res.deltaLutUpdated ? 1 : 0);
}

void Service::handleEvent(const CustomMessage::TrackCalibrate& event)
{
    if (!mAwaitingCalibration) {
        return;   // no pending session (e.g. a short run finalised on stop)
    }
    finalizeActivity(event.distanceActualM);
}

void Service::handleEvent(const CustomMessage::CalibDataRequest& /*event*/)
{
    using namespace SDK::Calibration;

    // Read the shared outdoor LUT fresh from disk (the on-disk state is the
    // source of truth for the View Data screen) and summarise it for the GUI.
    StrideLut lut;
    lut.loadFromFile(mKernel.fs, StrideLut::kDefaultPath);  // absent -> all-zero

    constexpr uint16_t kN = StrideLut::kBinCount;
    uint8_t pct[kN] = {};
    for (uint16_t i = 0; i < kN; ++i) {
        float p = (Config::kBinValidMinSteps > 0.0f)
            ? (lut.bin(i).total_steps / Config::kBinValidMinSteps) * 100.0f
            : 0.0f;
        if (p > 100.0f) p = 100.0f;
        pct[i] = static_cast<uint8_t>(p + 0.5f);
    }
    const uint8_t status = lut.readyForPhase2()          ? 2
                         : lut.readyForOutdoorEstimate()  ? 1
                                                          : 0;
    mGuiSender.calibData(status, static_cast<uint16_t>(lut.validBinCount()), kN,
                         lut.totalCalibrationDistanceM(), pct, kN);
}

void Service::handleEvent(const CustomMessage::CalibClear& /*event*/)
{
    using namespace SDK::Calibration;
    SDK::Interface::IFileSystem& fs = mKernel.fs;

    // Back up then drop a store so the next session starts uncalibrated. Only
    // remove the live file once the backup copy has succeeded, so a copy
    // failure never destroys data without a recoverable backup.
    auto backupAndRemove = [&fs](const char* live, const char* backup) {
        if (!fs.exist(live)) {
            return;
        }
        if (fs.copy(live, backup)) {
            fs.remove(live);
            LOG_INFO("Calib clear: %s -> %s\n", live, backup);
        } else {
            LOG_WARNING("Calib clear: backup of %s failed; live file kept\n", live);
        }
    };

    backupAndRemove(StrideLut::kDefaultPath, "../SharedData/stride_deleted.json");
    backupAndRemove(Config::kDeltaLutPath,   "treadmill_delta_deleted.json");
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

        mActivityWriter.pause(mTimeCounter.getCurrent());

        mEstimator.pause();   // halt integration; speed -> 0 (overrides hold)

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

        mActivityWriter.resume(mTimeCounter.getCurrent());

        mEstimator.resume();   // resume integration; hold state cleared
        mLastTickUtc = 0;      // avoid a spurious cross-pause delta_t

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

void Service::onWristTilt(uint32_t timestampMs)
{
    LOG_DEBUG("Wrist Tilt detected\n");
    backlightOn();
}


// =============================================================================
// Interval training state machine
// =============================================================================

void Service::handleEvent(const CustomMessage::IntervalsNextPhase& event)
{
    if (mIntervalsMode && mTrackState == Track::State::ACTIVE) {
        advanceIntervalsPhase(true);
    }
}

void Service::emitIntervalsWorkout()
{
    const Settings::Intervals& cfg = mSettings.intervals;

    auto runDuration = [&](ActivityWriter::WorkoutStepData& s) {
        if (cfg.runMetric == Settings::Intervals::TIME && cfg.runTime > 0) {
            s.durationType = SDK::Fit::WktStepDuration::Time;
            s.durationValue = static_cast<uint32_t>(cfg.runTime) * 1000u;        // ms
        } else if (cfg.runMetric == Settings::Intervals::DISTANCE && cfg.runDistance > 0.0f) {
            s.durationType = SDK::Fit::WktStepDuration::Distance;
            s.durationValue = static_cast<uint32_t>(cfg.runDistance * 100.0f);   // cm
        } else {
            s.durationType = SDK::Fit::WktStepDuration::Open;
            s.durationValue = 0;
        }
    };
    auto restDuration = [&](ActivityWriter::WorkoutStepData& s) {
        if (cfg.restMetric == Settings::Intervals::TIME && cfg.restTime > 0) {
            s.durationType = SDK::Fit::WktStepDuration::Time;
            s.durationValue = static_cast<uint32_t>(cfg.restTime) * 1000u;        // ms
        } else if (cfg.restMetric == Settings::Intervals::DISTANCE && cfg.restDistance > 0.0f) {
            s.durationType = SDK::Fit::WktStepDuration::Distance;
            s.durationValue = static_cast<uint32_t>(cfg.restDistance * 100.0f);   // cm
        } else {
            s.durationType = SDK::Fit::WktStepDuration::Open;
            s.durationValue = 0;
        }
    };

    ActivityWriter::WorkoutStepData steps[8];
    uint8_t n = 0;
    IntervalsStepMap map{};

    // Warm up
    if (cfg.warmUp) {
        steps[n].intensity = SDK::Fit::Intensity::Warmup;
        steps[n].durationType = SDK::Fit::WktStepDuration::Open;
        map.warmUpIdx = n;
        n++;
    }

    // Run (first / repeated run)
    map.runIdx = n;
    steps[n].intensity = SDK::Fit::Intensity::Active;
    runDuration(steps[n]);
    n++;

    if (cfg.repeatsNum == 0) {
        // 'Open' (unlimited): describe a single run + rest pair, no terminating repeat.
        map.restIdx = n;
        steps[n].intensity = SDK::Fit::Intensity::Rest;
        restDuration(steps[n]);
        n++;
        map.finalRunIdx = map.runIdx;
    } else if (cfg.lastRest) {
        // Every rep has a rest: repeat the run+rest pair repeatsNum times.
        map.restIdx = n;
        steps[n].intensity = SDK::Fit::Intensity::Rest;
        restDuration(steps[n]);
        n++;
        if (cfg.repeatsNum >= 2) {
            steps[n].durationType = SDK::Fit::WktStepDuration::RepeatUntilStepsComplete;
            steps[n].durationValue = map.runIdx;            // loop back to the run step
            steps[n].repeatCount = cfg.repeatsNum;          // total iterations
            n++;
        }
        map.finalRunIdx = map.runIdx;
    } else {
        // Skip the final rest: repeat run+rest (repeatsNum - 1) times, then a final run.
        if (cfg.repeatsNum >= 2) {
            map.restIdx = n;
            steps[n].intensity = SDK::Fit::Intensity::Rest;
            restDuration(steps[n]);
            n++;
            steps[n].durationType = SDK::Fit::WktStepDuration::RepeatUntilStepsComplete;
            steps[n].durationValue = map.runIdx;
            steps[n].repeatCount = static_cast<uint32_t>(cfg.repeatsNum - 1);
            n++;
            map.finalRunIdx = n;
            steps[n].intensity = SDK::Fit::Intensity::Active;
            runDuration(steps[n]);
            n++;
        } else {
            // repeatsNum == 1 with the rest skipped: a single run, no rest.
            map.finalRunIdx = map.runIdx;
        }
    }

    // Cool down
    if (cfg.coolDown) {
        steps[n].intensity = SDK::Fit::Intensity::Cooldown;
        steps[n].durationType = SDK::Fit::WktStepDuration::Open;
        map.coolDownIdx = n;
        n++;
    }

    map.valid = true;
    mIntervalsStepMap = map;

    mActivityWriter.addWorkout("Intervals", steps, n);
}

uint16_t Service::intervalsWktStepIndex() const
{
    if (!mIntervalsStepMap.valid) {
        return 0xFFFF;
    }

    const Track::IntervalsData& iv = mTrackData.intervals;
    const Settings::Intervals& cfg = mSettings.intervals;

    switch (iv.phase) {
    case Track::IntervalsPhase::WARM_UP:   return mIntervalsStepMap.warmUpIdx;
    case Track::IntervalsPhase::COOL_DOWN: return mIntervalsStepMap.coolDownIdx;
    case Track::IntervalsPhase::REST:      return mIntervalsStepMap.restIdx;
    case Track::IntervalsPhase::RUN:
        // The final run uses a standalone step only when the last rest is skipped.
        if (!cfg.lastRest && cfg.repeatsNum >= 2 && iv.repeat >= cfg.repeatsNum) {
            return mIntervalsStepMap.finalRunIdx;
        }
        return mIntervalsStepMap.runIdx;
    }
    return 0xFFFF;
}

void Service::startIntervalsPhase(Track::IntervalsPhase phase)
{
    mPhaseStartActiveSec  = mTimeCounter.getValueActive();
    mPhaseStartActiveDist = mDistanceCounter.getValueActive();

    Track::IntervalsData& iv   = mTrackData.intervals;
    const Settings::Intervals& cfg = mSettings.intervals;

    LOG_DEBUG("Intervals cfg: repeats=%u warmUp=%u coolDown=%u "
             "runMetric=%u runTime=%u runDist=%.1f "
             "restMetric=%u restTime=%u restDist=%.1f\n",
             cfg.repeatsNum, cfg.warmUp, cfg.coolDown,
             cfg.runMetric, cfg.runTime, cfg.runDistance,
             cfg.restMetric, cfg.restTime, cfg.restDistance);

    iv.phase = phase;

    switch (phase) {
    case Track::IntervalsPhase::WARM_UP:
    case Track::IntervalsPhase::COOL_DOWN:
        iv.metric        = Track::IntervalsMetric::TIME_OPEN;
        iv.phaseTimerSec = 0;
        iv.distRemaining = 0.0f;
        break;

    case Track::IntervalsPhase::RUN:
        if (cfg.runMetric == Settings::Intervals::TIME && cfg.runTime > 0) {
            iv.metric        = Track::IntervalsMetric::TIME_REMAINING;
            iv.phaseTimerSec = static_cast<time_t>(cfg.runTime);
            iv.distRemaining = 0.0f;
        } else if (cfg.runMetric == Settings::Intervals::DISTANCE && cfg.runDistance > 0.0f) {
            iv.metric        = Track::IntervalsMetric::DISTANCE;
            iv.phaseTimerSec = 0;
            iv.distRemaining = cfg.runDistance;
        } else {
            iv.metric        = Track::IntervalsMetric::TIME_OPEN;
            iv.phaseTimerSec = 0;
            iv.distRemaining = 0.0f;
        }
        break;

    case Track::IntervalsPhase::REST:
        if (cfg.restMetric == Settings::Intervals::TIME && cfg.restTime > 0) {
            iv.metric        = Track::IntervalsMetric::TIME_REMAINING;
            iv.phaseTimerSec = static_cast<time_t>(cfg.restTime);
            iv.distRemaining = 0.0f;
        } else if (cfg.restMetric == Settings::Intervals::DISTANCE && cfg.restDistance > 0.0f) {
            iv.metric        = Track::IntervalsMetric::DISTANCE;
            iv.phaseTimerSec = 0;
            iv.distRemaining = cfg.restDistance;
        } else {
            iv.metric        = Track::IntervalsMetric::TIME_OPEN;
            iv.phaseTimerSec = 0;
            iv.distRemaining = 0.0f;
        }
        break;
    }

    LOG_DEBUG("Intervals: phase=%u metric=%u repeat=%u/%u\n",
              static_cast<uint8_t>(iv.phase),
              static_cast<uint8_t>(iv.metric),
              iv.repeat, iv.totalRepeats);
}

void Service::processIntervals()
{
    if (mIntervalsCompleted) {
        return;
    }

    Track::IntervalsData& iv   = mTrackData.intervals;
    const Settings::Intervals& cfg = mSettings.intervals;

    const time_t elapsed = mTimeCounter.getValueActive() - mPhaseStartActiveSec;
    bool autoAdvance = false;

    switch (iv.metric) {
    case Track::IntervalsMetric::TIME_OPEN:
        iv.phaseTimerSec = elapsed;
        break;

    case Track::IntervalsMetric::TIME_ELAPSED:
        iv.phaseTimerSec = elapsed;
        break;

    case Track::IntervalsMetric::TIME_REMAINING: {
        const time_t target = (iv.phase == Track::IntervalsPhase::RUN)
            ? static_cast<time_t>(cfg.runTime)
            : static_cast<time_t>(cfg.restTime);
        iv.phaseTimerSec = (elapsed < target) ? (target - elapsed) : 0;
        autoAdvance = (elapsed >= target);
        break;
    }

    case Track::IntervalsMetric::DISTANCE: {
        const float target = (iv.phase == Track::IntervalsPhase::RUN)
            ? cfg.runDistance
            : cfg.restDistance;
        const float done = mDistanceCounter.getValueActive() - mPhaseStartActiveDist;
        iv.distRemaining = (done < target) ? (target - done) : 0.0f;
        autoAdvance = (done >= target);
        break;
    }
    }

    if (autoAdvance) {
        advanceIntervalsPhase();
    }
}

void Service::advanceIntervalsPhase(bool manual)
{
    Track::IntervalsData& iv       = mTrackData.intervals;
    const Settings::Intervals& cfg = mSettings.intervals;

    const Track::IntervalsPhase prevPhase = iv.phase;
    Track::IntervalsPhase nextPhase       = iv.phase;
    bool completed = false;

    switch (iv.phase) {
    case Track::IntervalsPhase::WARM_UP:
        nextPhase = Track::IntervalsPhase::RUN;
        iv.repeat = 1;
        break;

    case Track::IntervalsPhase::RUN:
        if (!cfg.lastRest && cfg.repeatsNum != 0 && iv.repeat >= cfg.repeatsNum) {
            // User chose to skip the rest after the final interval: go straight
            // to cool down (or finish if cool down is disabled). Not applicable
            // to 'Open' repeats (repeatsNum == 0), which have no final interval.
            if (cfg.coolDown) {
                nextPhase = Track::IntervalsPhase::COOL_DOWN;
            } else {
                completed = true;
            }
        } else {
            // Go to REST; REST decides whether to cycle again or finish.
            nextPhase = Track::IntervalsPhase::REST;
        }
        break;

    case Track::IntervalsPhase::REST:
        // Semantics: repeatsNum = number of RUN-REST cycles the user selected.
        //   repeatsNum=N (N>=1)  ->  exactly N cycles
        //   repeatsNum=0         ->  'Open': cycle indefinitely until the user ends
        //                            the workout manually (never auto-completes here)
        if (cfg.repeatsNum == 0 || iv.repeat < cfg.repeatsNum) {
            iv.repeat++;
            nextPhase = Track::IntervalsPhase::RUN;
        } else {
            if (cfg.coolDown) {
                nextPhase = Track::IntervalsPhase::COOL_DOWN;
            } else {
                completed = true;
            }
        }
        break;

    case Track::IntervalsPhase::COOL_DOWN:
        completed = true;
        break;
    }

    if (completed) {
        LOG_INFO("Intervals: workout completed\n");

        // The programmed workout is done, but the session keeps running so the
        // user can record additional manual laps and end it themselves. Close the
        // final phase as a lap, then drop out of intervals mode so the track
        // screen reverts to normal faces and R2 records a lap (not a phase advance).
        if (mLapNotEmpty) {
            saveLap();
        }
        mIntervalsCompleted = true;
        mIntervalsMode = false;
        mTrackData.intervalsMode = false;

        // alert=true: user needs notification when workout ends automatically (REST->END).
        // COOL_DOWN->END is always manual, so alert&&!manual stays false -> no vibro.
        onIntervalsPhaseChange(true, manual);
        mGuiSender.intervalsWorkoutCompleted();
        return;
    }

    // Record the phase that just finished as its own lap before the next one
    // begins. The final phase is captured by stopTrack() when the workout ends.
    if (mLapNotEmpty) {
        saveLap();
    }

    startIntervalsPhase(nextPhase);

    const bool isActivePhase = (nextPhase == Track::IntervalsPhase::RUN ||
                                nextPhase == Track::IntervalsPhase::REST);

    // WARM_UP -> RUN always shows an alert (marks workout start, user needs to see the phase),
    // even though the transition is manual.
    // All other transitions show an alert only on auto-advance.
    const bool isWarmupToRun  = (prevPhase == Track::IntervalsPhase::WARM_UP &&
                                 nextPhase == Track::IntervalsPhase::RUN);

    const bool showAlertScreen = isWarmupToRun || 
                                    isActivePhase ||
                                    (!manual && nextPhase == Track::IntervalsPhase::COOL_DOWN);

    if (showAlertScreen) {
        mGuiSender.intervalsPhaseAlert(mTrackData.intervals);
    }

    onIntervalsPhaseChange(showAlertScreen, manual);
}

void Service::onIntervalsPhaseChange(bool alert, bool manual)
{
    LOG_DEBUG("Intervals: phase change (phase=%u, alert=%u, manual=%u)\n",
             static_cast<uint8_t>(mTrackData.intervals.phase),
             static_cast<uint8_t>(alert),
             static_cast<uint8_t>(manual));

    if (alert) {
        backlightOn(skBacklightTimeout * 2); // covers both the alert screen and the next one
    }

    // fire vibro/buzzer - alert screen shown on auto-advance,
    // user needs active notification to attract attention.
    if (alert && !manual) {
        playVibroPattern(SDK::Message::RequestVibroPlay::Effect::SHORT_DOUBLE_CLICK_STRONG_1_100);
        playBuzzerPattern(150, 2);
    }
}
