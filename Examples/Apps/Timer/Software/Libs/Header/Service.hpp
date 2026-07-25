#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"

#include "TimerManager.hpp"
#include "Commands.hpp"

class Service : public TimerManager::TimerCallback
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

    // -- Timer & persistence --------------------------------------------------

    TimerManager    mTimerManager;
    Timer           mActiveTimer;
    bool            mTimeFormat12h = false;  // cached system clock-format setting

    // -- Lifecycle ------------------------------------------------------------

    void onStartGUI();
    void onStopGUI();

    /** @brief Query the kernel for the current 12/24-hour clock setting. */
    void refreshTimeFormat();

    // -- Fired control ------------------------------------------------------

    void stopFired();

    // -- Event handlers -------------------------------------------------------

    void handleEvent(const CustomMessage::TimerList& event);
    void handleEvent(const CustomMessage::TimerActivateEffect& event);
    void handleEvent(const CustomMessage::TimerStop& event);
    void handleEvent(const CustomMessage::TimerStopAll& event);
    void handleEvent(const CustomMessage::TimerSnooze& event);
    void handleEvent(const CustomMessage::TimerSnoozeAll& event);

    // -- TimerManager callbacks -----------------------------------------------

    void onTimer(const Timer& timer);
    void onListChanged(const std::vector<Timer>& list);
};

#endif // SERVICE_HPP
