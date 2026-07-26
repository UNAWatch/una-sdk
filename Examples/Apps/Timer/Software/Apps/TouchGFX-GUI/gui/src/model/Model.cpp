#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#include <algorithm>

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

// Preset durations (seconds) shown at the top of the Main list.
static constexpr uint16_t kPresetSec[] = { 60, 180, 300, 600, 900, 1800, 3600 };

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    , mSrvSender(mKernel)
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

    setCapabilities();
    buildPresets();

#if defined(SIMULATOR)
    LOG_INFO("Simulator.\n");
    LOG_INFO("Buttons: 1=L1 2=L2 3=R1 4=R2\n\n");
#endif
}


// -- Controls -----------------------------------------------------------------

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

        // Any interaction means the user is in the app; return to Main on exit
        // rather than leaving the app entirely.
        if (mState != TimerState::FIRED) {
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
    if (mState == TimerState::FIRED) {
        application().gotoFiredScreenNoTransition();
        return;
    }

    if (mState == TimerState::RUNNING || mState == TimerState::PAUSED) {
        application().gotoRunningScreenNoTransition();
        return;
    }

    if (mStayInApp) {
        mStayInApp = false;
        application().gotoMainScreenNoTransition();
        return;
    }

    exitApp();
}


// -- Countdown ----------------------------------------------------------------

void Model::startTimer(const Timer& timer)
{
    mSrvSender.start(timer.durationSec, timer.effect);
}

void Model::pauseTimer()  { mSrvSender.pause();  }
void Model::resumeTimer() { mSrvSender.resume(); }
void Model::resetTimer()  { mSrvSender.reset();  }
void Model::stopTimer()   { mSrvSender.stop();   }
void Model::repeatTimer() { mSrvSender.repeat(); }

uint32_t Model::getRemainingMs() const
{
    switch (mState) {
        case TimerState::RUNNING: {
            int32_t left = static_cast<int32_t>(mEndTick - mKernel.sys.getTimeMs());
            return left > 0 ? static_cast<uint32_t>(left) : 0;
        }
        case TimerState::PAUSED:
            return mRemainingMs;
        case TimerState::FIRED:
            return 0;
        case TimerState::IDLE:
        default:
            return static_cast<uint32_t>(mDurationSec) * 1000u;
    }
}


// -- Presets & recents --------------------------------------------------------

void Model::buildPresets()
{
    mPresets.clear();
    mPresets.reserve(sizeof(kPresetSec) / sizeof(kPresetSec[0]));
    for (uint16_t sec : kPresetSec) {
        mPresets.push_back(Timer{ sec, Timer::EFFECT_BEEP_AND_VIBRO });
    }
}

void Model::addRecent(const Timer& timer)
{
    // Move-to-front with de-duplication, newest first.
    mRecents.erase(std::remove(mRecents.begin(), mRecents.end(), timer), mRecents.end());
    mRecents.insert(mRecents.begin(), timer);

    if (mRecents.size() > CustomMessage::kMaxRecents) {
        mRecents.resize(CustomMessage::kMaxRecents);
    }

    mSrvSender.saveRecents(mRecents);
}

void Model::removeRecent(const Timer& timer)
{
    const size_t before = mRecents.size();
    mRecents.erase(std::remove(mRecents.begin(), mRecents.end(), timer), mRecents.end());

    // A preset (not in recents) leaves the list unchanged -- nothing to persist.
    if (mRecents.size() != before) {
        mSrvSender.saveRecents(mRecents);
    }
}


// -- Private ------------------------------------------------------------------

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


// -- IGuiLifeCycleCallback ----------------------------------------------------

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


// -- ICustomMessageHandler ----------------------------------------------------

bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::TIMER_STATE: {
            auto* m = static_cast<CustomMessage::TimerStateMsg*>(msg);
            mState       = static_cast<TimerState>(m->state);
            mEndTick     = m->endTick;
            mRemainingMs = m->remainingMs;
            mDurationSec = m->durationSec;
            mEffect      = m->effect;
            modelListener->onStateChanged();
        } break;

        case CustomMessage::TIMER_FIRED: {
            auto* m = static_cast<CustomMessage::TimerFired*>(msg);
            mState       = TimerState::FIRED;
            mRemainingMs = 0;
            mDurationSec = m->durationSec;
            mEffect      = m->effect;
            modelListener->onFired(Timer{ m->durationSec, m->effect });
        } break;

        case CustomMessage::TIMER_RECENTS: {
            auto* m = static_cast<CustomMessage::TimerRecents*>(msg);
            mRecents.clear();
            mRecents.reserve(m->count);
            for (uint8_t i = 0; i < m->count && i < CustomMessage::kMaxRecents; ++i) {
                mRecents.push_back(Timer{ m->entries[i].durationSec, m->entries[i].effect });
            }
            modelListener->onRecentsChanged(mRecents);
        } break;

        default:
            break;
    }

    return true;
}
