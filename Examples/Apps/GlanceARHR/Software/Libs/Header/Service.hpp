#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include <ctime>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Glance/GlanceControl.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service();

    void run();

private:
    void connect();
    void disconnect();

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);

    void glanceUpdate();

    void onGlanceTick();
    bool configGui();
    void createGuiControls();

private:
    static constexpr int16_t kTitleX = 18;
    static constexpr int16_t kTitleY = 0;
    static constexpr int16_t kTitleW = 205;
    static constexpr int16_t kTitleH = 30;

    static constexpr int16_t kValueAHRX = 44;
    static constexpr int16_t kValueAHRY = 28;
    static constexpr int16_t kValueAHRW = 80;
    static constexpr int16_t kValueAHRH = 40;

    static constexpr int16_t kValueRHRX = 114;
    static constexpr int16_t kValueRHRY = 28;
    static constexpr int16_t kValueRHRW = 80;
    static constexpr int16_t kValueRHRH = 40;

    const SDK::Kernel&       mKernel;
    SDK::Glance::Form        mGlanceUI;
    SDK::Glance::ControlText mGlanceTitle;
    SDK::Glance::ControlText mGlanceValueAHR;
    SDK::Glance::ControlText mGlanceValueRHR;

    SDK::Sensor::Connection mSensorHRMetrics;
    uint32_t                mAvgHrValue;
    uint32_t                mRestHrValue;
    bool                    mDataReceived;
};

#endif
