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
    // SDK::Sensor::Connection  mSensorHR;  // Commented out: HR sensor connection for hello world tutorial
    // float                    mHR;         // Commented out: Heart rate value
    // float                    mHRTL;       // Commented out: Heart rate trust level
    // ActivityWriter           mActivityWriter;  // Commented out: FIT file writer

    void onStartGUI();
    void onStopGUI();

    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data);

    static uint32_t ParseVersion(const char* str);
};

#endif
