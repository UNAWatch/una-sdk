#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include <vector>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <memory>
#include <inttypes.h>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Glance/GlanceControl.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

class Service : public SDK::Interface::ISensorDataListener
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    // ===== SENSOR MANAGEMENT =====
    void connect();
    void disconnect();

    // ISensorDataListener implementation
    void onSdlNewData(uint16_t                 handle,
                      const SDK::Sensor::Data* data,
                      uint16_t                 count,
                      uint16_t                 stride) override;

    // ===== GLANCE UI =====
    void onGlanceTick();
    bool configGui();
    void createGuiControls();

    // ===== SESSION MANAGEMENT =====
    void startSession();
    void finalizeSession();

    // ===== FIT FILE MANAGEMENT =====
    void saveFit(bool finalize);
    void appendPendingRecords();
    void writeFitDefinitions(std::time_t timestamp);
    void writeFitSessionSummary(std::time_t timestamp);
    void writeStepsFieldDescription();

    struct FitRecord {
        std::time_t timestamp;
        uint8_t     heartRate;
        uint32_t    steps;
    };

    const SDK::Kernel&       mKernel;
    SDK::Glance::Form        mGlanceUI;
    SDK::Glance::ControlText mGlanceTitle;
    SDK::Glance::ControlText mGlanceValue;
    SDK::Glance::ControlText mGlanceHR;

    SDK::Sensor::Connection mSensorSteps;
    SDK::Sensor::Connection mSensorHR;

    bool mGlanceActive = false;

    // ===== SESSION STATE =====
    // Accumulators for current session
    uint32_t mTotalSteps = 0;
    uint32_t mLastSteps = 0;
    uint32_t mSampleCount = 0;
    uint8_t mCurrentHR = 0;
    std::vector<FitRecord> mPendingRecords;
    bool mSessionOpen = false;
    std::time_t mSessionStart = 0;

    // ===== FIT ENCODER =====
    // Native SDK::Fit streaming encoder. Constructed in saveFit() over the open
    // file; emits definition/data records directly and is reset after finish().
    std::unique_ptr<SDK::Fit::FitWriter> mFit;

    // Local message types (0-15) associated with each FIT message definition.
    enum Local : uint8_t {
        L_FILE_ID    = 0,
        L_DEV_ID     = 1,
        L_FIELD_DESC = 2,
        L_EVENT      = 3,
        L_RECORD     = 4,
        L_SESSION    = 5,
        L_ACTIVITY   = 6,
    };

    // ===== CONSTANTS =====
    // Developer field number for the "steps" custom field on the record message.
    static constexpr uint8_t skStepsDevFieldNum = 0;

    static constexpr uint32_t skSamplePeriodSec = 5;
    static constexpr const char* skFitFileName = "steps.fit";
};

#endif