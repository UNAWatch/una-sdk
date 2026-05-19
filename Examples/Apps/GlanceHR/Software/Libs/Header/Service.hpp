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

    static constexpr int16_t kIconX = 39;
    static constexpr int16_t kIconY = 31;

    static constexpr int16_t kCalculatingValueX = 85;
    static constexpr int16_t kCalculatingValueY = 30;
    static constexpr int16_t kCalculatingValueW = 150;
    static constexpr int16_t kCalculatingValueH = 40;

    static constexpr int16_t kValidValueX = 70;
    static constexpr int16_t kValidValueY = 28;
    static constexpr int16_t kValidValueW = 100;
    static constexpr int16_t kValidValueH = 40;

    static constexpr char skTextCalculating[] = "Calculating...";

    const SDK::Kernel&        mKernel;
    SDK::Glance::Form         mGlanceUI;
    SDK::Glance::ControlText  mGlanceTitle;
    SDK::Glance::ControlText  mGlanceValue;
    SDK::Glance::ControlImage mIcon;

    SDK::Sensor::Connection mSensorHR;
    uint32_t                mHrValue;

    enum class OutFormat {
      NONE = 0,
      CALCULATION,
      HR,
    };
    OutFormat mOutFormat;
};

#endif
