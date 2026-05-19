
#include "Service.hpp"

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
        , mAlarmManager(kernel)
        , mActiveAlarm()
{
}

Service::~Service()
{
    mAlarmManager.attachCallback(nullptr);
}

void Service::run()
{
    LOG_INFO("Started\n");

    mAlarmManager.attachCallback(this);
    mAlarmManager.load();

    uint32_t startTime = mKernel.sys.getTimeMs();

    while (true) {
        uint32_t sleepTime = mAlarmManager.execute(getLocalTime());

        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, sleepTime)) {
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    mAlarmManager.attachCallback(nullptr);
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
                case CustomMessage::ALARM_LIST:
                    LOG_DEBUG("ALARM_LIST\n");
                    handleEvent(*static_cast<CustomMessage::AlarmList*>(msg));
                    break;

                case CustomMessage::ACTIVATED_EFFECT:
                    LOG_DEBUG("ACTIVATED_EFFECT\n");
                    handleEvent(*static_cast<CustomMessage::AlarmActivateEffect*>(msg));
                    break;

                case CustomMessage::ALARM_STOP:
                    LOG_DEBUG("ALARM_STOP\n");
                    handleEvent(*static_cast<CustomMessage::AlarmStop*>(msg));
                    break;

                case CustomMessage::ALARM_STOP_ALL:
                    LOG_DEBUG("ALARM_STOP_ALL\n");
                    handleEvent(*static_cast<CustomMessage::AlarmStopAll*>(msg));
                    break;

                case CustomMessage::ALARM_SNOOZE:
                    LOG_DEBUG("ALARM_SNOOZE\n");
                    handleEvent(*static_cast<CustomMessage::AlarmSnooze*>(msg));
                    break;

                case CustomMessage::ALARM_SNOOZE_ALL:
                    LOG_DEBUG("ALARM_SNOOZE_ALL\n");
                    handleEvent(*static_cast<CustomMessage::AlarmSnoozeAll*>(msg));
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
                if (!mAlarmManager.hasActiveAlarms()) {
                    LOG_INFO("No active alarms and GUI not started, exiting service\n");
                    mAlarmManager.attachCallback(nullptr);
                    return; // Exit app
                }
            }
        }
    }
}

void Service::onStartGUI()
{
    mGuiStarted = true;

    // If there is an active alarm, send it to GUI first
    if (mActiveAlarm.on) {
        mGuiSender.alarmActivated(mActiveAlarm);
        mActiveAlarm = {}; // clear active alarm
    }

    // Send current alarm list to GUI
    mGuiSender.listUpd(mAlarmManager.getAlarmList());
}

void Service::onStopGUI()
{
    mGuiStarted = false;
}

void Service::handleEvent(const CustomMessage::AlarmList& event)
{
    mAlarmManager.saveAlarmList({event.alarms, event.alarms + event.count});
}

void Service::handleEvent(const CustomMessage::AlarmActivateEffect& event)
{
    auto *backlightMsg = mKernel.comm.allocateMessage<SDK::Message::RequestBacklightSet>();
    if (backlightMsg) {
        backlightMsg->autoOffTimeoutMs = 4000;
        backlightMsg->brightness = 100;
        mKernel.comm.sendMessage(backlightMsg);
        mKernel.comm.releaseMessage(backlightMsg);
    }

    bool isVibro = event.alarm.effect == Alarm::Effect::EFFECT_BEEP_AND_VIBRO ||
            event.alarm.effect == Alarm::Effect::EFFECT_VIBRO;

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

    bool isBuzzer = event.alarm.effect == Alarm::Effect::EFFECT_BEEP_AND_VIBRO ||
            event.alarm.effect == Alarm::Effect::EFFECT_BEEP;

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

void Service::handleEvent(const CustomMessage::AlarmStop& event)
{
    mAlarmManager.disableAlarm(event.alarm);
    stopRinging();
}

void Service::handleEvent(const CustomMessage::AlarmStopAll& /*event*/)
{
    mAlarmManager.disableAllActiveAlarm();
    stopRinging();
}

void Service::handleEvent(const CustomMessage::AlarmSnooze& event)
{
    mAlarmManager.snoozeAlarm(event.alarm);
    stopRinging();
}

void Service::handleEvent(const CustomMessage::AlarmSnoozeAll& /*event*/)
{
    mAlarmManager.snoozeAllActiveAlarm();
    stopRinging();
}

void Service::stopRinging()
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

void Service::onAlarm(const Alarm& alarm)
{
    // Make sure GUI is started
    if (!mGuiStarted) {
        auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestAppRunGui>();
        if (msg) {
            mKernel.comm.sendMessage(msg);
            mKernel.comm.releaseMessage(msg);
        }

        mActiveAlarm = alarm; // save active alarm
    } else {
        mGuiSender.alarmActivated(alarm);
        mActiveAlarm = {};
    }
}

void Service::onListChanged(const std::vector<Alarm>& list)
{
    if (mGuiStarted) {
        mGuiSender.listUpd(list);
    }
}

