#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include <vector>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <inttypes.h>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Glance/GlanceControl.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/FitHelper/FitHelper.hpp"
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
    void appendPendingRecords(SDK::Interface::IFile* fp);
    void writeFitDefinitions(SDK::Interface::IFile* fp, std::time_t timestamp);
    void writeFitSessionSummary(SDK::Interface::IFile* fp, std::time_t timestamp);

    struct FitRecord {
        std::time_t timestamp;
        uint8_t     heartRate;
        uint32_t    steps;
    };

    const SDK::Kernel&       mKernel;
    const char*              mName;
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
    bool mFitFileInitialized = false;

    // ===== FIT HELPERS =====
    SDK::Component::FitHelper mFitFileID;
    SDK::Component::FitHelper mFitDeveloper;
    SDK::Component::FitHelper mFitRecord;
    SDK::Component::FitHelper mFitEvent;
    SDK::Component::FitHelper mFitSession;
    SDK::Component::FitHelper mFitActivity;
    SDK::Component::FitHelper mFitStepsField;

    // ===== CONSTANTS =====
    static constexpr uint8_t skFileMsgNum     = 1;
    static constexpr uint8_t skDevelopMsgNum  = 2;
    static constexpr uint8_t skRecordMsgNum   = 3;
    static constexpr uint8_t skEventMsgNum    = 4;
    static constexpr uint8_t skSessionMsgNum  = 5;
    static constexpr uint8_t skActivityMsgNum = 6;
    static constexpr uint8_t skStepsMsgNum    = 7;

    static constexpr uint32_t skSamplePeriodSec = 5;
    static constexpr const char* skFitFileName = "steps.fit";
};

#endif