/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   Waypoint background logic: read the configuration, follow the GPS.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "AppConfigFields.hpp"
#include "Commands.hpp"

#include "SDK/AppConfig/AppConfig.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include <memory>

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    void onStartGUI();
    void onStopGUI();
    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch &data);

    /// Load every configured value into the members the screen is fed from.
    void loadConfiguration();

    /// Store the current fix as the new target and write it back to the file.
    void saveTargetHere();

    void sendNavUpdate();

    SDK::Kernel             &mKernel;
    SDK::Sensor::Connection  mSensorGPS;

    /**
     * @brief   Read once when the service starts, and again after a write-back.
     *
     * Created in run(), not in the constructor. SDK::AppConfig logs when a file
     * is unusable, and in the simulator the service is constructed before
     * TouchGFX's HAL exists -- which is what the logger writes through, so
     * logging that early crashes the simulator.
     */
    std::unique_ptr<SDK::AppConfig> mConfig;

    // -- Configured values -------------------------------------------------
    char    mWaypointName[WaypointConfig::kNameBytes];
    float   mTargetLatitude;
    float   mTargetLongitude;
    int32_t mArrivalRadiusM;
    bool    mVibrateOnArrival;
    bool    mTargetIsConfigured;

    // -- Live state --------------------------------------------------------
    bool  mGUIStarted;
    bool  mHasFix;
    float mLatitude;
    float mLongitude;
    float mDistanceM;
    float mBearingDeg;
    /// Latched so arriving buzzes once, not once per fix.
    bool  mArrivalAnnounced;
};

#endif // SERVICE_HPP
