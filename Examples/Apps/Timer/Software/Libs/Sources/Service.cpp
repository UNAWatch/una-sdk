#include "Service.hpp"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include <cstdio>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

// Grace period after launch during which the service stays alive waiting for
// the GUI to appear, even with no active countdown.
static constexpr uint32_t kStartupGraceMs = 5000;

// Upper bound on how long a background fire waits for the GUI it launched to
// come up and consume it. If the launch is dropped or the GUI never signals
// COMMAND_APP_NOTIF_GUI_RUN, the pending delivery is dropped so the service can
// go idle and exit instead of blocking forever.
static constexpr uint32_t kFiredDeliveryTimeoutMs = 10000;


Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mTimerManager(kernel)
        , mWidget(kernel)
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

        // A fire waiting for the GUI must not pin the service alive forever: if
        // the launched GUI never comes up to consume it, drop the pending
        // delivery so the idle-exit below can trip. While still waiting, keep the
        // loop ticking so the deadline is actually checked.
        if (mPendingFired) {
            // Wrap-safe elapsed. onFired stamps mPendingFiredTick from getTimeMs()
            // *after* `now` was sampled at the top of the loop, so on the firing
            // iteration the stamp can sit a hair ahead of `now`; an unsigned
            // (now - tick) would then underflow to a huge value and drop the fire
            // we just armed. The signed difference stays correctly negative.
            if (static_cast<int32_t>(now - mPendingFiredTick) > static_cast<int32_t>(kFiredDeliveryTimeoutMs)) {
                LOG_INFO("GUI never consumed the fire, dropping pending delivery\n");
                mPendingFired = false;
            } else if (sleepTime > kFiredDeliveryTimeoutMs) {
                sleepTime = kFiredDeliveryTimeoutMs;
            }
        }

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

        // Reflect the countdown on the home screen (may shorten sleepTime so the
        // MM:SS ticks each second while running).
        pumpWidget(now, sleepTime);

        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, sleepTime)) {
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    stopEffect();     // silence a FIRED alert before we go away
                    mWidget.stop();   // leave no stale widget on the home screen
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

    // Deliver a fire that happened while the GUI was closed FIRST, so the GUI
    // routes straight to the Fired screen. A state update sent before it would
    // route the blank Startup screen to Main, flashing it before Fired takes over.
    if (mPendingFired) {
        // GUI was closed -> background fire
        SDK::send_msg<CustomMessage::TimerFired>(mKernel, mPendingTimer, true);
        mPendingFired = false;
    }

    // Bring the GUI up to date: current countdown, then recents.
    sendStateToGui();
    SDK::send_msg<CustomMessage::TimerRecents>(mKernel, mTimerManager.getRecents());
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
        case CustomMessage::TimerCmd::REPLAY_ALERT:
            // Re-indication only: replay the alert, state is unchanged (FIRED),
            // so there is nothing new to report to the GUI.
            playEffect(mTimerManager.getState().effect);
            return;
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
    SDK::send_msg<CustomMessage::TimerStateMsg>(mKernel, s.state, s.endTick, s.remainingMs,
                                                s.durationSec, s.effect);
}

void Service::pumpWidget(uint32_t now, uint32_t& sleepTime)
{
    const TimerManager::State s = mTimerManager.getState();
    // Only surface the widget while the app is backgrounded: with the GUI open
    // the user already sees the Running screen, so opening it hides the widget
    // and closing it (countdown still going) brings it back.
    const bool active = !mGuiStarted &&
                        (s.state == TimerState::RUNNING || s.state == TimerState::PAUSED);

    if (!active) {
        if (mWidgetActive) {
            mWidget.stop();
            mWidgetActive  = false;
            mLastWidgetSec = -1;
        }
        return;
    }

    if (!mWidgetActive) {
        mWidget.start();
        mWidgetActive  = true;
        mLastWidgetSec = -1;   // force the first update below
    }

    // Remaining ms: extrapolated from the shared tick while running, frozen when
    // paused. Wrap-safe via the signed difference.
    uint32_t remMs;
    if (s.state == TimerState::RUNNING) {
        const int32_t left = static_cast<int32_t>(s.endTick - now);
        remMs = (left > 0) ? static_cast<uint32_t>(left) : 0u;

        // Wake on the next whole-second boundary so MM:SS ticks crisply.
        uint32_t toNextSec = remMs % 1000u;
        if (toNextSec == 0u) {
            toNextSec = 1000u;
        }
        if (toNextSec < sleepTime) {
            sleepTime = toNextSec;
        }
    } else {
        remMs = s.remainingMs;
    }

    // Only push when the shown second changes (so a paused timer pushes once).
    const int32_t secs = static_cast<int32_t>((remMs + 999u) / 1000u);   // ceil
    if (secs == mLastWidgetSec) {
        return;
    }
    mLastWidgetSec = secs;

    // percent = remaining share of the duration. WIDGET_PROGRESS_FROM_END pins the
    // fill to the bar's end, so it empties from the start as the count runs down.
    const uint32_t totalMs = static_cast<uint32_t>(s.durationSec) * 1000u;
    float percent = 0.0f;
    if (totalMs > 0u) {
        const uint32_t remClamped = (remMs < totalMs) ? remMs : totalMs;
        percent = 100.0f * static_cast<float>(remClamped) / static_cast<float>(totalMs);
    }

    const unsigned mm = (static_cast<unsigned>(secs) / 60u) % 100u;   // MM:SS, minutes 0..99
    const unsigned ss =  static_cast<unsigned>(secs) % 60u;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02u:%02u", mm, ss);
    mWidget.update(SDK::Message::WIDGET_SHOW_TEXT | SDK::Message::WIDGET_SHOW_PERCENT |
                       SDK::Message::WIDGET_PROGRESS_FROM_END,
                   percent, buf);
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
        // GUI is up -> fired in-app
        SDK::send_msg<CustomMessage::TimerFired>(mKernel, timer, false);
    } else {
        // Launch the GUI, then deliver the fire once it signals it is running.
        auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestAppRunGui>();
        if (msg) {
            mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }
        mPendingFired     = true;
        mPendingTimer     = timer;
        mPendingFiredTick = mKernel.sys.getTimeMs();
    }
}

void Service::onRecentsChanged(const std::vector<Timer>& list)
{
    if (mGuiStarted) {
        SDK::send_msg<CustomMessage::TimerRecents>(mKernel, list);
    }
}
