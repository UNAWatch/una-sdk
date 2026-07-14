#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include "Commands.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    Service(SDK::Kernel& kernel);

    virtual ~Service() = default;

    void run();

private:
    SDK::Kernel&             mKernel;
    CustomMessage::GUISender mSender;
    bool                     mGUIStarted;
    SDK::Sensor::Connection  mSensorHR;
    SDK::Sensor::Connection  mSensorGPS;
    SDK::Sensor::Connection  mSensorAltimeter;
    SDK::Sensor::Connection  mSensorAccelerometer;
    SDK::Sensor::Connection  mSensorStepCounter;
    SDK::Sensor::Connection  mSensorFloorCounter;
    SDK::Sensor::Connection  mSensorMagneticField;
    SDK::Sensor::Connection  mSensorBattery;
    SDK::Sensor::Connection  mSensorAccelerometerRaw;
    SDK::Sensor::Connection  mSensorGyroscope;
    SDK::Sensor::Connection  mSensorGyroscopeRaw;
    SDK::Sensor::Connection  mSensorHeartBeat;
    SDK::Sensor::Connection  mSensorHeartRateMetrics;
    SDK::Sensor::Connection  mSensorStepDetector;
    SDK::Sensor::Connection  mSensorAmbientTemperature;
    SDK::Sensor::Connection  mSensorPressure;
    SDK::Sensor::Connection  mSensorWristMotion;
    SDK::Sensor::Connection  mSensorMotionDetect;
    SDK::Sensor::Connection  mSensorActivityRecognition;
    SDK::Sensor::Connection  mSensorGestureRecognition;
    SDK::Sensor::Connection  mSensorActivity;
    SDK::Sensor::Connection  mSensorPPG;
    SDK::Sensor::Connection  mSensorECG;
    SDK::Sensor::Connection  mSensorGPSSpeed;
    SDK::Sensor::Connection  mSensorGPSDistance;
    SDK::Sensor::Connection  mSensorBatteryCharging;
    SDK::Sensor::Connection  mSensorBatteryMetrics;
    SDK::Sensor::Connection  mSensorFusion;
    SDK::Sensor::Connection  mSensorFusionRaw;
    SDK::Sensor::Connection  mSensorTouchDetect;
    float                    mHR;
    float                    mHRTL;
    // CPU time tracking
    uint32_t                 mServiceCpuTimeMs;
    uint32_t                 mGuiCpuTimeMs;
    uint32_t                 mActiveTimeMs;
    // Message rate tracking
    uint32_t                 mTxMessages;
    uint32_t                 mRxMessages;
    uint32_t                 mTxBytes;
    uint32_t                 mRxBytes;
    uint32_t                 mLastStatsTimeMs;
    uint32_t                 mLastAccTimeMs;
    uint32_t                 mLastMagTimeMs;

    void onStartGUI();
    void onStopGUI();

    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data);

    static uint32_t ParseVersion(const char* str);
};

#endif
