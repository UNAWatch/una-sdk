#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

// Grace period after launch during which the service stays alive waiting for
// the GUI to appear, even with no active countdown.
static constexpr uint32_t kStartupGraceMs = 5000;


Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mGuiSender(kernel)
        , mTimerManager(kernel)
{
}

Service::~Service()
{
    mTimerManager.attachCallback(nullptr);
}

void Service::run()
{
    LOG_INFO("Started\n");

    mTimerManager.attachCallback(this);
    mTimerManager.load();

    const uint32_t startTime = mKernel.sys.getTimeMs();

    while (true) {
        uint32_t now      = mKernel.sys.getTimeMs();
        uint32_t sleepTime = mTimerManager.execute(now);

        // Nothing keeps us alive: no GUI and no armed / paused countdown and no
        // fire waiting to be delivered. Exit after the startup grace period;
        // during the grace period poll instead of blocking forever.
        if (!mGuiStarted && !mTimerManager.hasActiveTimers() && !mPendingFired) {
            if (now - startTime > kStartupGraceMs) {
                LOG_INFO("Idle and GUI closed, exiting service\n");
                break;
            }
            if (sleepTime > kStartupGraceMs) {
                sleepTime = kStartupGraceMs;
            }
        }

        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, sleepTime)) {
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mTimerManager.attachCallback(nullptr);
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    onStartGUI();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    onStopGUI();
                    break;

                // Custom messages (GUI -> Service)
                case CustomMessage::TIMER_START:
                    handleStart(*static_cast<CustomMessage::TimerStart*>(msg));
                    break;

                case CustomMessage::TIMER_CONTROL:
                    handleControl(*static_cast<CustomMessage::TimerControl*>(msg));
                    break;

                case CustomMessage::TIMER_RECENTS_SAVE:
                    handleRecentsSave(*static_cast<CustomMessage::TimerRecentsSave*>(msg));
                    break;

                default:
                    break;
            }
            mKernel.comm.releaseMessage(msg);
        }
    }
}


// -- Lifecycle ----------------------------------------------------------------

void Service::onStartGUI()
{
    mGuiStarted = true;

    // Bring the GUI up to date: current countdown, then recents.
    sendStateToGui();
    mGuiSender.sendRecents(mTimerManager.getRecents());

    // Deliver a fire that happened while the GUI was closed.
    if (mPendingFired) {
        mGuiSender.fired(mPendingTimer, true);   // GUI was closed -> background fire
        mPendingFired = false;
    }
}

void Service::onStopGUI()
{
    mGuiStarted = false;
}


// -- Command handlers ---------------------------------------------------------

void Service::handleStart(const CustomMessage::TimerStart& msg)
{
    mTimerManager.start(msg.durationSec, msg.effect, mKernel.sys.getTimeMs());
    sendStateToGui();
}

void Service::handleControl(const CustomMessage::TimerControl& msg)
{
    uint32_t now = mKernel.sys.getTimeMs();

    switch (msg.cmd) {
        case CustomMessage::TimerCmd::PAUSE:
            mTimerManager.pause(now);
            break;
        case CustomMessage::TimerCmd::RESUME:
            mTimerManager.resume(now);
            break;
        case CustomMessage::TimerCmd::RESET:
            mTimerManager.reset(now);
            break;
        case CustomMessage::TimerCmd::STOP:
            stopEffect();
            mTimerManager.stop();
            break;
        case CustomMessage::TimerCmd::REPEAT:
            stopEffect();
            mTimerManager.repeat(now);
            break;
    }

    sendStateToGui();
}

void Service::handleRecentsSave(const CustomMessage::TimerRecentsSave& msg)
{
    std::vector<Timer> list;
    list.reserve(msg.count);
    for (uint8_t i = 0; i < msg.count && i < CustomMessage::kMaxRecents; ++i) {
        list.push_back(Timer{ msg.entries[i].durationSec, msg.entries[i].effect });
    }
    mTimerManager.saveRecents(list);
}


// -- Helpers ------------------------------------------------------------------

void Service::sendStateToGui()
{
    if (!mGuiStarted) {
        return;
    }
    TimerManager::State s = mTimerManager.getState();
    mGuiSender.sendState(s.state, s.endTick, s.remainingMs, s.durationSec, s.effect);
}

void Service::playEffect(Timer::Effect effect)
{
    auto *backlightMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBacklightSet>();
    if (backlightMsg) {
        backlightMsg->autoOffTimeoutMs = 4000;
        backlightMsg->brightness = 100;
        mKernel.comm.sendMessage(backlightMsg);
        mKernel.comm.releaseMessage(backlightMsg);
    }

    bool isVibro = effect == Timer::EFFECT_BEEP_AND_VIBRO ||
                   effect == Timer::EFFECT_VIBRO;

    if (isVibro) {
        auto *vibroMsg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
        if (vibroMsg) {
            vibroMsg->notes[0].effect = SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100;
            vibroMsg->notes[1].pause  = 250;
            vibroMsg->notes[2].effect = SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100;
            vibroMsg->notes[3].pause  = 250;
            vibroMsg->notes[4].effect = SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100;
            vibroMsg->notesCount = 5;

            mKernel.comm.sendMessage(vibroMsg);
            mKernel.comm.releaseMessage(vibroMsg);
        }
    }

    bool isBuzzer = effect == Timer::EFFECT_BEEP_AND_VIBRO ||
                    effect == Timer::EFFECT_BEEP;

    if (isBuzzer) {
        auto *buzzerMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
        if (buzzerMsg) {
            buzzerMsg->notes[0].volume = 100;
            buzzerMsg->notes[0].time   = 750;
            buzzerMsg->notes[1].volume = 0;
            buzzerMsg->notes[1].time   = 250;
            buzzerMsg->notes[2].volume = 100;
            buzzerMsg->notes[2].time   = 750;
            buzzerMsg->notes[3].volume = 0;
            buzzerMsg->notes[3].time   = 250;
            buzzerMsg->notes[4].volume = 100;
            buzzerMsg->notes[4].time   = 750;
            buzzerMsg->notesCount = 5;

            mKernel.comm.sendMessage(buzzerMsg);
            mKernel.comm.releaseMessage(buzzerMsg);
        }
    }
}

void Service::stopEffect()
{
    // Empty play messages silence the buzzer and stop the vibro pattern.
    auto *buzzerMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
    if (buzzerMsg) {
        mKernel.comm.sendMessage(buzzerMsg);
        mKernel.comm.releaseMessage(buzzerMsg);
    }

    auto *vibroMsg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (vibroMsg) {
        mKernel.comm.sendMessage(vibroMsg);
        mKernel.comm.releaseMessage(vibroMsg);
    }
}


// -- TimerManager callbacks ---------------------------------------------------

void Service::onFired(const Timer& timer)
{
    playEffect(timer.effect);

    if (mGuiStarted) {
        mGuiSender.fired(timer, false);   // GUI is up -> fired in-app
    } else {
        // Launch the GUI, then deliver the fire once it signals it is running.
        auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestAppRunGui>();
        if (msg) {
            mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        mPendingFired = true;
        mPendingTimer = timer;
    }
}

void Service::onRecentsChanged(const std::vector<Timer>& list)
{
    if (mGuiStarted) {
        mGuiSender.sendRecents(list);
    }
}
