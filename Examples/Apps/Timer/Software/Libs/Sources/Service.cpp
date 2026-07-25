
#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#include <cstdio>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

static std::tm getLocalTime()
{
    std::tm tmResult {};
    std::time_t utc = time(nullptr);

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmResult, &utc);
#else
    localtime_r(&utc, &tmResult);
#endif

    return tmResult;
}

Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mGuiSender(kernel)
        , mTimerManager(kernel)
        , mActiveTimer()
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

    uint32_t startTime = mKernel.sys.getTimeMs();

    while (true) {
        uint32_t sleepTime = mTimerManager.execute(getLocalTime());

        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, sleepTime)) {
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mTimerManager.attachCallback(nullptr);
                    // We must release message because this is the last event.
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

                // Custom messages
                case CustomMessage::TIMER_LIST:
                    LOG_DEBUG("TIMER_LIST\n");
                    handleEvent(*static_cast<CustomMessage::TimerList*>(msg));
                    break;

                case CustomMessage::ACTIVATED_EFFECT:
                    LOG_DEBUG("ACTIVATED_EFFECT\n");
                    handleEvent(*static_cast<CustomMessage::TimerActivateEffect*>(msg));
                    break;

                case CustomMessage::TIMER_STOP:
                    LOG_DEBUG("TIMER_STOP\n");
                    handleEvent(*static_cast<CustomMessage::TimerStop*>(msg));
                    break;

                case CustomMessage::TIMER_STOP_ALL:
                    LOG_DEBUG("TIMER_STOP_ALL\n");
                    handleEvent(*static_cast<CustomMessage::TimerStopAll*>(msg));
                    break;

                case CustomMessage::TIMER_SNOOZE:
                    LOG_DEBUG("TIMER_SNOOZE\n");
                    handleEvent(*static_cast<CustomMessage::TimerSnooze*>(msg));
                    break;

                case CustomMessage::TIMER_SNOOZE_ALL:
                    LOG_DEBUG("TIMER_SNOOZE_ALL\n");
                    handleEvent(*static_cast<CustomMessage::TimerSnoozeAll*>(msg));
                    break;

                default:
                    break;
            }
            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        if (!mGuiStarted) {
            // Just wait some time to see if GUI starts
            if (mKernel.sys.getTimeMs() - startTime > 5000) {
                if (!mTimerManager.hasActiveTimers()) {
                    LOG_INFO("No active timers and GUI not started, exiting service\n");
                    mTimerManager.attachCallback(nullptr);
                    return; // Exit app
                }
            }
        }
    }
}

void Service::refreshTimeFormat()
{
    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(100) && msg.ok()) {
            mTimeFormat12h = msg->timeFormat;
        }
    }
}

void Service::onStartGUI()
{
    mGuiStarted = true;

    // Pick up the current clock-format setting so the list carries it to the GUI.
    refreshTimeFormat();

    // If there is an active timer, send it to GUI first
    if (mActiveTimer.on) {
        mGuiSender.timerActivated(mActiveTimer);
        mActiveTimer = {}; // clear active timer
    }

    // Send current timer list to GUI
    mGuiSender.listUpd(mTimerManager.getTimerList(), mTimeFormat12h);
}

void Service::onStopGUI()
{
    mGuiStarted = false;
}

void Service::handleEvent(const CustomMessage::TimerList& event)
{
    mTimerManager.saveTimerList({event.timers, event.timers + event.count});
}

void Service::handleEvent(const CustomMessage::TimerActivateEffect& event)
{
    auto *backlightMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBacklightSet>();
    if (backlightMsg) {
        backlightMsg->autoOffTimeoutMs = 4000;
        backlightMsg->brightness = 100;
        mKernel.comm.sendMessage(backlightMsg);
        mKernel.comm.releaseMessage(backlightMsg);
    }

    bool isVibro = event.timer.effect == Timer::Effect::EFFECT_BEEP_AND_VIBRO ||
            event.timer.effect == Timer::Effect::EFFECT_VIBRO;

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

    bool isBuzzer = event.timer.effect == Timer::Effect::EFFECT_BEEP_AND_VIBRO ||
            event.timer.effect == Timer::Effect::EFFECT_BEEP;

    if (isBuzzer) {
        auto *buzzerMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
        if (buzzerMsg) {
            buzzerMsg->notes[0].volume = 100;
            buzzerMsg->notes[0].time  = 750;
            buzzerMsg->notes[1].volume = 0;
            buzzerMsg->notes[1].time  = 250;
            buzzerMsg->notes[2].volume = 100;
            buzzerMsg->notes[2].time  = 750;
            buzzerMsg->notes[3].volume = 0;
            buzzerMsg->notes[3].time  = 250;
            buzzerMsg->notes[4].volume = 100;
            buzzerMsg->notes[4].time  = 750;
            buzzerMsg->notesCount = 5;

            mKernel.comm.sendMessage(buzzerMsg);
            mKernel.comm.releaseMessage(buzzerMsg);
        }
    }
}

void Service::handleEvent(const CustomMessage::TimerStop& event)
{
    mTimerManager.disableTimer(event.timer);
    stopFired();
}

void Service::handleEvent(const CustomMessage::TimerStopAll& /*event*/)
{
    mTimerManager.disableAllActiveTimer();
    stopFired();
}

void Service::handleEvent(const CustomMessage::TimerSnooze& event)
{
    mTimerManager.snoozeTimer(event.timer);
    stopFired();
}

void Service::handleEvent(const CustomMessage::TimerSnoozeAll& /*event*/)
{
    mTimerManager.snoozeAllActiveTimer();
    stopFired();
}

void Service::stopFired()
{
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

void Service::onTimer(const Timer& timer)
{
    // Make sure GUI is started
    if (!mGuiStarted) {
        auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestAppRunGui>();
        if (msg) {
            mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }

        mActiveTimer = timer; // save active timer
    } else {
        mGuiSender.timerActivated(timer);
        mActiveTimer = {};
    }
}

void Service::onListChanged(const std::vector<Timer>& list)
{
    if (mGuiStarted) {
        mGuiSender.listUpd(list, mTimeFormat12h);
    }
}

