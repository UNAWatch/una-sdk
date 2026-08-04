#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#include <algorithm>

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

// Preset durations (seconds) shown at the top of the Main list.
static constexpr uint16_t kPresetSec[] = { 60, 180, 300, 600, 900, 1800, 3600 };

// Every countdown control is the same one-field message; only the sub-command differs.
static void sendControl(const SDK::Kernel& kernel, CustomMessage::TimerCmd cmd)
{
    SDK::send_msg<CustomMessage::TimerControl>(kernel, cmd);
}

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
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

    // Any button activity keeps the app awake -- clicks and the press/release
    // pair a hold sends, so playing with the wheel does not idle out mid-hold.
    if (SDK::GUI::Button::isButtonCode(key)) {
        resetIdleTimer();
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


// -- Countdown ----------------------------------------------------------------

void Model::startTimer(const Timer& timer)
{
    SDK::send_msg<CustomMessage::TimerStart>(mKernel, timer.durationSec, timer.effect);

    // Remember custom and modified-preset timers as recents; an unchanged preset
    // is already offered in the list, so it is not duplicated.
    if (!isPreset(timer)) {
        addRecent(timer);
    }
}

bool Model::isPreset(const Timer& timer) const
{
    return std::find(mPresets.begin(), mPresets.end(), timer) != mPresets.end();
}

int16_t Model::editTimerIndex() const
{
    // Mirror the Main wheel order: New(0), presets, then recents.
    int16_t idx = 1;
    for (const auto& p : mPresets) {
        if (p == mEditTimer) { return idx; }
        ++idx;
    }
    for (const auto& r : mRecents) {
        if (r == mEditTimer) { return idx; }
        ++idx;
    }
    return 0;   // not in either list -> New
}

void Model::pauseTimer()  { sendControl(mKernel, CustomMessage::TimerCmd::PAUSE);  }
void Model::resumeTimer() { sendControl(mKernel, CustomMessage::TimerCmd::RESUME); }
void Model::resetTimer()  { sendControl(mKernel, CustomMessage::TimerCmd::RESET);  }
void Model::stopTimer()   { sendControl(mKernel, CustomMessage::TimerCmd::STOP);   }
void Model::repeatTimer() { sendControl(mKernel, CustomMessage::TimerCmd::REPEAT); }
void Model::replayAlert() { sendControl(mKernel, CustomMessage::TimerCmd::REPLAY_ALERT); }

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

    SDK::send_msg<CustomMessage::TimerRecentsSave>(mKernel, mRecents);
}

void Model::removeRecent(const Timer& timer)
{
    const size_t before = mRecents.size();
    mRecents.erase(std::remove(mRecents.begin(), mRecents.end(), timer), mRecents.end());

    // A preset (not in recents) leaves the list unchanged -- nothing to persist.
    if (mRecents.size() != before) {
        SDK::send_msg<CustomMessage::TimerRecentsSave>(mKernel, mRecents);
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
        msg->enMusicControl      = false;
        msg->enUsbChargingScreen = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
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

            // First state after launch: route away from the blank Startup screen
            // to the right destination, so Main's "New" face never flashes before
            // a cold-start countdown takes over. (A cold-start fire arrives as
            // TIMER_FIRED instead and is routed there.)
            if (!mStartupRouted) {
                mStartupRouted = true;
                if (mState == TimerState::RUNNING || mState == TimerState::PAUSED) {
                    // Seed the edit timer from the live countdown so its Menu and
                    // the Main selection resolve to it, not the stale default.
                    mEditTimer = Timer{ mDurationSec, mEffect };
                    application().gotoRunningScreenNoTransition();
                } else {
                    application().gotoMainScreenNoTransition();
                }
            }
        } break;

        case CustomMessage::TIMER_FIRED: {
            auto* m = static_cast<CustomMessage::TimerFired*>(msg);
            mState                = TimerState::FIRED;
            mRemainingMs          = 0;
            mDurationSec          = m->durationSec;
            mEffect               = m->effect;
            mFiredFromBackground  = m->background;
            // A fire is also a valid first-message-after-launch (cold-start into a
            // fired alarm): onFired routes to the Fired screen, so mark startup
            // routed to keep a later idle state from re-routing to Main.
            mStartupRouted = true;
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
