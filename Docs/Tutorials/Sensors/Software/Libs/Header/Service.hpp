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
    float                    mHR;
    float                    mHRTL;
    // CPU time tracking
    uint32_t                 mServiceCpuTimeMs;
    uint32_t                 mGuiCpuTimeMs;
    // Message rate tracking
    uint32_t                 mTxMessages;
    uint32_t                 mRxMessages;
    uint32_t                 mTxBytes;
    uint32_t                 mRxBytes;
    uint32_t                 mLastStatsTimeMs;
    uint32_t                 mLastAccTimeMs;

    void onStartGUI();
    void onStopGUI();

    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data);

    static uint32_t ParseVersion(const char* str);
};

#endif
