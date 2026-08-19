/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   Waypoint background logic: read the configuration, follow the GPS.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include <cmath>
#include <cstring>

namespace {

constexpr float kEarthRadiusM = 6371000.0f;

/// How far past the arrival radius the user must get before arriving again.
constexpr float kReArmFactor = 1.5f;
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
constexpr float kRadToDeg = 180.0f / 3.14159265358979323846f;

/// Great-circle distance in metres. Haversine, which stays well behaved at the
/// short ranges a waypoint is used over.
float distanceMetres(float latA, float lonA, float latB, float lonB)
{
    const float dLat = (latB - latA) * kDegToRad;
    const float dLon = (lonB - lonA) * kDegToRad;
    const float sinLat = std::sin(dLat * 0.5f);
    const float sinLon = std::sin(dLon * 0.5f);
    const float a = sinLat * sinLat +
            std::cos(latA * kDegToRad) * std::cos(latB * kDegToRad) *
            sinLon * sinLon;
    return 2.0f * kEarthRadiusM * std::asin(std::sqrt(a < 1.0f ? a : 1.0f));
}

/// Initial great-circle bearing, degrees clockwise from true north.
float bearingDegrees(float latA, float lonA, float latB, float lonB)
{
    const float latAr = latA * kDegToRad;
    const float latBr = latB * kDegToRad;
    const float dLon = (lonB - lonA) * kDegToRad;
    const float y = std::sin(dLon) * std::cos(latBr);
    const float x = std::cos(latAr) * std::sin(latBr) -
            std::sin(latAr) * std::cos(latBr) * std::cos(dLon);
    float bearing = std::atan2(y, x) * kRadToDeg;
    if (bearing < 0.0f) {
        bearing += 360.0f;
    }
    return bearing;
}

} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSensorGPS(SDK::Sensor::Type::GPS_LOCATION, 0, 0)
    , mWaypointName {}
    , mTargetLatitude(0.0f)
    , mTargetLongitude(0.0f)
    , mArrivalRadiusM(0)
    , mVibrateOnArrival(false)
    , mTargetIsConfigured(false)
    , mGUIStarted(false)
    , mHasFix(false)
    , mLatitude(0.0f)
    , mLongitude(0.0f)
    , mDistanceM(0.0f)
    , mBearingDeg(0.0f)
    , mArrivalAnnounced(false)
{
    (void)kernel;
}

void Service::loadConfiguration()
{
    if (!mConfig) {
        return;
    }

    mConfig->getString("waypointName", mWaypointName, sizeof(mWaypointName));
    mTargetLatitude = mConfig->getFloat("targetLatitude");
    mTargetLongitude = mConfig->getFloat("targetLongitude");
    mArrivalRadiusM = mConfig->getInt("arrivalRadiusM");
    mVibrateOnArrival = mConfig->getBool("vibrateOnArrival");

    // has() separates "the user chose this" from "this is our default", so the
    // screen can say so instead of navigating confidently to a made-up place.
    mTargetIsConfigured = mConfig->has("targetLatitude") &&
            mConfig->has("targetLongitude");

    LOG_INFO("target '%s' %s, radius %ldm, vibrate %s\n",
             mWaypointName,
             mTargetIsConfigured ? "from configuration" : "NOT configured (default)",
             static_cast<long>(mArrivalRadiusM),
             mVibrateOnArrival ? "on" : "off");
}

void Service::run()
{
    LOG_INFO("thread started\n");

    // Read the configuration here rather than in the constructor. The file the
    // companion app wrote is read once, so a change made on the phone applies
    // the next time this app starts.
    mConfig.reset(new SDK::AppConfig(mKernel, WaypointConfig::kFileName,
                                     WaypointConfig::kFields,
                                     WaypointConfig::kFieldCount));
    loadConfiguration();

    mSensorGPS.connect();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                // Release the GPS explicitly rather than leaving it to the
                // Connection destructor: this is the last message before the
                // app is torn down, so the cleanup belongs here where it is
                // visible.
                mSensorGPS.disconnect();
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                onStartGUI();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will ever reclaim the thread. Waypoint has no
                // reason to exist without its screen: staying alive would hold
                // the GPS subscription open and keep buzzing on arrival after
                // the user has left the app.
                onStopGUI();
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto event = static_cast<SDK::Message::Sensor::EventData *>(msg);
                SDK::Sensor::DataBatch data(event->data, event->count,
                                            event->stride);
                onSdlNewData(event->handle, data);
            } break;

            case CustomMessage::SAVE_TARGET_HERE:
                saveTargetHere();
                break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }

    LOG_INFO("thread stopped\n");
}

void Service::onStartGUI()
{
    mGUIStarted = true;
    // Give the screen the configured target straight away, so it has something
    // to show before the first fix arrives.
    sendNavUpdate();
}

void Service::onStopGUI()
{
    mGUIStarted = false;
    // Release the GPS so it is not left powered by an app the user has closed.
    mSensorGPS.disconnect();
}

void Service::onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    if (!mSensorGPS.matchesDriver(handle)) {
        return;
    }

    SDK::SensorDataParser::GpsLocation parser(data[0]);
    if (!parser.isDataValid()) {
        return;
    }

    // isDataValid() only says the sample is a well-formed GPS record;
    // isCoordinatesValid() is the fix itself. Without this second check a
    // fix-less sample reads as latitude 0, longitude 0 -- a point in the Gulf of
    // Guinea -- and the screen briefly shows a confident distance to it.
    if (!parser.isCoordinatesValid()) {
        // The fix is gone (or has not arrived yet). Say so rather than leaving a
        // stale distance on screen looking live -- the position itself is kept,
        // so a brief dropout does not lose the target.
        if (mHasFix) {
            mHasFix = false;
            mArrivalAnnounced = false;
            if (mGUIStarted) {
                sendNavUpdate();
            }
        }
        return;
    }

    mLatitude = parser.getLatitude();
    mLongitude = parser.getLongitude();
    mHasFix = true;

    mDistanceM = distanceMetres(mLatitude, mLongitude,
                                mTargetLatitude, mTargetLongitude);
    mBearingDeg = bearingDegrees(mLatitude, mLongitude,
                                 mTargetLatitude, mTargetLongitude);

    const float radiusM = static_cast<float>(mArrivalRadiusM);
    const bool arrived = mDistanceM <= radiusM;

    if (arrived && !mArrivalAnnounced) {
        mArrivalAnnounced = true;
        if (mVibrateOnArrival) {
            auto vibro = SDK::make_msg<SDK::Message::RequestVibroPlay>(mKernel);
            if (vibro) {
                vibro->notes[0].effect =
                        SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100;
                vibro->notesCount = 1;
                vibro.send();
            }
        }
        LOG_INFO("arrived at '%s'\n", mWaypointName);
    } else if (mArrivalAnnounced && mDistanceM > radiusM * kReArmFactor) {
        // Re-arm once the user has genuinely left, so a second approach buzzes
        // again. The margin matters: a fix sitting near the radius jitters
        // either side of it by more than a GPS error, and re-arming on the
        // first sample outside would buzz over and over without it.
        mArrivalAnnounced = false;
    }

    if (mGUIStarted) {
        sendNavUpdate();
    }
}

void Service::saveTargetHere()
{
    if (!mConfig) {
        return;
    }

    if (!mHasFix) {
        LOG_WARNING("no fix yet; target unchanged\n");
        SDK::send_msg<CustomMessage::TargetSaved>(mKernel, false, mTargetLatitude,
                                                  mTargetLongitude);
        return;
    }

    // Write the new target back into the same file the companion app owns. The
    // setters clamp to the declared bounds, and save() swaps the file in
    // atomically, so an interrupted write cannot lose the previous target.
    mConfig->setFloat("targetLatitude", mLatitude);
    mConfig->setFloat("targetLongitude", mLongitude);

    if (!mConfig->save()) {
        LOG_WARNING("could not save the new target\n");
        // The setters already updated the in-memory values, so re-reading now
        // would adopt coordinates that never reached the file -- the screen
        // would navigate to a target the next launch will not know about. Keep
        // serving the previous one and report the failure with it.
        SDK::send_msg<CustomMessage::TargetSaved>(mKernel, false, mTargetLatitude,
                                                  mTargetLongitude);
        return;
    }

    // Re-read so the values in play are exactly what is now on disk.
    loadConfiguration();

    mDistanceM = 0.0f;
    mBearingDeg = 0.0f;
    mArrivalAnnounced = false;

    SDK::send_msg<CustomMessage::TargetSaved>(mKernel, true, mTargetLatitude,
                                              mTargetLongitude);
    sendNavUpdate();
}

void Service::sendNavUpdate()
{
    auto update = SDK::make_msg<CustomMessage::NavUpdate>(mKernel);
    if (!update) {
        return;
    }

    CustomMessage::NavState &nav = update->nav;
    std::strncpy(nav.waypointName, mWaypointName,
                 sizeof(nav.waypointName) - 1);
    nav.waypointName[sizeof(nav.waypointName) - 1] = '\0';
    nav.distanceM = mDistanceM;
    nav.bearingDeg = mBearingDeg;
    nav.targetLatitude = mTargetLatitude;
    nav.targetLongitude = mTargetLongitude;
    nav.arrivalRadiusM = mArrivalRadiusM;
    nav.hasFix = mHasFix;
    nav.arrived = mHasFix &&
            mDistanceM <= static_cast<float>(mArrivalRadiusM);
    nav.targetIsConfigured = mTargetIsConfigured;

    update.send();
}
