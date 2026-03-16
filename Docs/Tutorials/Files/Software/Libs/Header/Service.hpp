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

    /*
     * COMMENTED OUT: Heart Rate Sensor and FIT Logging Setup
     * To enable HR monitoring and FIT file logging:
     * 1. Uncomment the lines below
     * 2. Include necessary headers in Service.cpp:
     *    #include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
     * 3. Uncomment sensor connect/disconnect in Service::run()
     * 4. Uncomment HR data processing in Service::onSdlNewData()
     * 5. Uncomment FIT logging in the main loop and exit
     * 6. Uncomment HR message handling in Model::customMessageHandler()
     * 7. Uncomment HR message definitions in Commands.hpp
     *
     * For adding new sensors:
     * - Add similar SDK::Sensor::Connection member
     * - Initialize in constructor with appropriate sensor type
     * - Add data parsing in onSdlNewData() with matchesDriver check
     * - Create new message types in Commands.hpp
     * - Handle in Model::customMessageHandler()
     */
    // SDK::Sensor::Connection  mSensorHR;  // HR sensor connection (1000ms sample, 2000ms timeout)
    // float                    mHR;         // Current heart rate value (BPM)
    // float                    mHRTL;       // Heart rate trust level (0.0-1.0)
    // ActivityWriter           mActivityWriter;  // FIT file writer for activity logging

    void onStartGUI();
    void onStopGUI();

    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data);

    static uint32_t ParseVersion(const char* str);
};

#endif
