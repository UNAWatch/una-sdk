#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "ActivityWriter.hpp"
#include "Commands.hpp"

class Service
{
public:
    Service(SDK::Kernel& kernel);

    virtual ~Service();

    void run();

private:
    // -- Constants ------------------------------------------------------------

    static constexpr uint32_t skSamplePeriod  = 1000;
    static constexpr uint32_t skSampleLatency = 2000;

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;
    CustomMessage::Sender mGuiSender;

    // -- Sensors --------------------------------------------------------------

    SDK::Sensor::Connection mSensorHr;

    // -- Metrics --------------------------------------------------------------

    float mHr;
    float mHrTl;

    // -- Activity -------------------------------------------------------------

    ActivityWriter mActivityWriter;

    // -- Lifecycle ------------------------------------------------------------

    void onStartGUI();
    void onStopGUI();

    // -- Sensor data dispatch -------------------------------------------------

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);
};

#endif // SERVICE_HPP
