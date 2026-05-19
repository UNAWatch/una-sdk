#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"

#include "AlarmManager.hpp"
#include "Commands.hpp"

class Service : public AlarmManager::AlarmCallback
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;
    CustomMessage::Sender mGuiSender;

    // -- Alarm & persistence --------------------------------------------------

    AlarmManager    mAlarmManager;
    Alarm           mActiveAlarm;

    // -- Lifecycle ------------------------------------------------------------

    void onStartGUI();
    void onStopGUI();

    // -- Ringing control ------------------------------------------------------

    void stopRinging();

    // -- Event handlers -------------------------------------------------------

    void handleEvent(const CustomMessage::AlarmList& event);
    void handleEvent(const CustomMessage::AlarmActivateEffect& event);
    void handleEvent(const CustomMessage::AlarmStop& event);
    void handleEvent(const CustomMessage::AlarmStopAll& event);
    void handleEvent(const CustomMessage::AlarmSnooze& event);
    void handleEvent(const CustomMessage::AlarmSnoozeAll& event);

    // -- AlarmManager callbacks -----------------------------------------------

    void onAlarm(const Alarm& alarm);
    void onListChanged(const std::vector<Alarm>& list);
};

#endif // SERVICE_HPP
