#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include "Commands.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "ActivityWriter.hpp"

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
    float                    mHR;
    float                    mHRTL;
    ActivityWriter           mActivityWriter;

    void onStartGUI();
    void onStopGUI();

    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data);

    static uint32_t ParseVersion(const char* str);
};

#endif
