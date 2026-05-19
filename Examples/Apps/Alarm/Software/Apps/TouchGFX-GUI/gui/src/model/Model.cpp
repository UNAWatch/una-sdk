#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    , mSrvSender(mKernel)
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

    setCapabilities();

#if defined(SIMULATOR)
    std::string fsPath = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Simulator.\n");
    LOG_INFO("FS path: [%s].\n", fsPath.c_str());
    LOG_INFO("Buttons: 1=L1 2=L2 3=R1 4=R2\n\n");
#endif
}


// Controls

FrontendApplication& Model::application()
{
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

void Model::tick()
{
    if (mIsRunning) {
        decIdleTimer();
    }

    if (mInvalidate) {
        mInvalidate = false;
        application().invalidate();
    }
}

void Model::handleKeyEvent(uint8_t key)
{
    LOG_DEBUG("key = %c\n", static_cast<char>(key));

    if (isAnyKeyPressed(key)) {
        resetIdleTimer();

        // If the user presses any key while no alarm is ringing, stay in app on exit
        if (!mActiveAlarm.on) {
            mStayInApp = true;
        }
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = App::Config::kScreenTimeoutSteps;
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");

    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);

    mKernel.sys.exit();
    // On simulator sys.exit() only sets a flag -- the current tick completes normally.
}

void Model::switchToNextPriorityScreen()
{
    if (mActiveAlarm.on) {
        application().gotoRingingScreenNoTransition();
        return;
    }

    if (mStayInApp) {
        mStayInApp = false;
        application().gotoMainScreenNoTransition();
        return;
    }

    exitApp();
}


// Alarm

const Alarm& Model::getActiveAlarm() const
{
    return mActiveAlarm;
}

void Model::playAlarm()
{
    LOG_DEBUG("called\n");
    mSrvSender.activateEffect(mActiveAlarm);
}

void Model::stopAlarm()
{
    LOG_DEBUG("called\n");
    mSrvSender.stopAll();
    mActiveAlarm = {};
}

void Model::snoozeAlarm()
{
    LOG_DEBUG("called\n");
    mSrvSender.snoozeAll();
    mActiveAlarm = {};
}

std::vector<Alarm>& Model::getAlarmList()
{
    return mAlarmList;
}

void Model::setAlarmEditId(size_t id)
{
    if (id > mAlarmList.size()) {  // id == mAlarmList.size() means new alarm
        return;
    }
    mEditAlarmId = id;
}

size_t Model::getAlarmEditId()
{
    return mEditAlarmId;
}

void Model::saveAlarm(size_t id, Alarm alarm)
{
    LOG_DEBUG("called\n");

    if (id < mAlarmList.size()) {
        mAlarmList[id] = alarm;
        mSrvSender.listUpd(mAlarmList);
        modelListener->onAlarmListUpdated(mAlarmList);
        return;
    }

    if (id == mAlarmList.size()) {
        // If an alarm with the same identity already exists, overwrite it to avoid duplicates
        for (size_t i = 0; i < mAlarmList.size(); i++) {
            if (mAlarmList[i] == alarm) {
                mEditAlarmId  = i;
                mAlarmList[i] = alarm;
                mSrvSender.listUpd(mAlarmList);
                modelListener->onAlarmListUpdated(mAlarmList);
                return;
            }
        }

        mAlarmList.push_back(alarm);
        mSrvSender.listUpd(mAlarmList);
    }
}

void Model::deleteAlarm(size_t id)
{
    if (id >= mAlarmList.size()) {
        return;
    }
    mAlarmList.erase(mAlarmList.begin() + id);
    mSrvSender.listUpd(mAlarmList);
    modelListener->onAlarmListUpdated(mAlarmList);
}


// Private

void Model::decIdleTimer()
{
    if (mIdleTimer > 0) {
        if (--mIdleTimer == 0) {
            modelListener->onIdleTimeout();
        }
    }
}

void Model::setCapabilities()
{
    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestSetCapabilities>();
    if (msg) {
        msg->enMusicControl      = true;
        msg->enUsbChargingScreen = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

bool Model::isAnyKeyPressed(uint8_t key) const
{
    return key == SDK::GUI::Button::L1 ||
           key == SDK::GUI::Button::L2 ||
           key == SDK::GUI::Button::R1 ||
           key == SDK::GUI::Button::R2;
}


// IGuiLifeCycleCallback

void Model::onStart()
{
    LOG_INFO("Started\n");
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
    mInvalidate = true;
}

void Model::onSuspend()
{
    mIsRunning = false;
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}


// ICustomMessageHandler

bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::ALARM_LIST: {
            LOG_DEBUG("ALARM_LIST\n");
            auto* m = static_cast<CustomMessage::AlarmList*>(msg);
            mAlarmList.assign(m->alarms, m->alarms + m->count);
            modelListener->onAlarmListUpdated(mAlarmList);
        } break;

        case CustomMessage::ACTIVATED_ALARM: {
            LOG_DEBUG("ACTIVATED_ALARM\n");
            auto* m  = static_cast<CustomMessage::ActivatedAlarm*>(msg);
            mActiveAlarm = m->alarm;
            modelListener->onAlarmActivated(mActiveAlarm);
        } break;

        default:
            break;
    }

    return true;
}
