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

        // If the user presses any key while no timer is fired, stay in app on exit
        if (!mActiveTimer.on) {
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
    if (mActiveTimer.on) {
        application().gotoFiredScreenNoTransition();
        return;
    }

    if (mStayInApp) {
        mStayInApp = false;
        application().gotoMainScreenNoTransition();
        return;
    }

    exitApp();
}


// Timer

const Timer& Model::getActiveTimer() const
{
    return mActiveTimer;
}

void Model::playTimer()
{
    LOG_DEBUG("called\n");
    mSrvSender.activateEffect(mActiveTimer);
}

void Model::stopTimer()
{
    LOG_DEBUG("called\n");
    mSrvSender.stopAll();
    mActiveTimer = {};
}

void Model::snoozeTimer()
{
    LOG_DEBUG("called\n");
    mSrvSender.snoozeAll();
    mActiveTimer = {};
}

std::vector<Timer>& Model::getTimerList()
{
    return mTimerList;
}

void Model::setTimerEditId(size_t id)
{
    if (id > mTimerList.size()) {  // id == mTimerList.size() means new timer
        return;
    }
    mEditTimerId = id;
}

size_t Model::getTimerEditId()
{
    return mEditTimerId;
}

void Model::saveTimer(size_t id, Timer timer)
{
    LOG_DEBUG("called\n");

    if (id < mTimerList.size()) {
        mTimerList[id] = timer;
        mSrvSender.listUpd(mTimerList);
        modelListener->onTimerListUpdated(mTimerList);
        return;
    }

    if (id == mTimerList.size()) {
        // If an timer with the same identity already exists, overwrite it to avoid duplicates
        for (size_t i = 0; i < mTimerList.size(); i++) {
            if (mTimerList[i] == timer) {
                mEditTimerId  = i;
                mTimerList[i] = timer;
                mSrvSender.listUpd(mTimerList);
                modelListener->onTimerListUpdated(mTimerList);
                return;
            }
        }

        mTimerList.push_back(timer);
        mSrvSender.listUpd(mTimerList);
    }
}

void Model::deleteTimer(size_t id)
{
    if (id >= mTimerList.size()) {
        return;
    }
    mTimerList.erase(mTimerList.begin() + id);
    mSrvSender.listUpd(mTimerList);
    modelListener->onTimerListUpdated(mTimerList);
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
        case CustomMessage::TIMER_LIST: {
            LOG_DEBUG("TIMER_LIST\n");
            auto* m = static_cast<CustomMessage::TimerList*>(msg);
            mTimerList.assign(m->timers, m->timers + m->count);
            mTimeFormat12h = m->timeFormat12h;
            modelListener->onTimerListUpdated(mTimerList);
        } break;

        case CustomMessage::ACTIVATED_TIMER: {
            LOG_DEBUG("ACTIVATED_TIMER\n");
            auto* m  = static_cast<CustomMessage::ActivatedTimer*>(msg);
            mActiveTimer = m->timer;
            modelListener->onTimerActivated(mActiveTimer);
        } break;

        default:
            break;
    }

    return true;
}
