#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"

#include "TimerManager.hpp"
#include "Commands.hpp"

/**
 * @brief Background process that owns the countdown and drives the alerts.
 *
 * Sleeps on the message queue with a timeout equal to the remaining countdown,
 * so the chip can reach low power while a timer runs. On expiry it plays the
 * alert (buzzer / vibro / backlight) and, if the GUI is closed, launches it to
 * show the Fired screen -- the timer therefore goes off in the app and on the
 * home screen alike.
 */
class Service : public TimerManager::Callback
{
public:
    explicit Service(SDK::Kernel &kernel);
    virtual ~Service();

    void run();

private:
    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;
    CustomMessage::Sender mGuiSender;
    TimerManager          mTimerManager;

    // A fire that happened while the GUI was closed: launch the GUI, then
    // deliver it once the GUI signals it is running.
    bool                  mPendingFired = false;
    Timer                 mPendingTimer {};

    // -- Lifecycle ------------------------------------------------------------

    void onStartGUI();
    void onStopGUI();

    // -- Command handlers (GUI -> Service) ------------------------------------

    void handleStart(const CustomMessage::TimerStart& msg);
    void handleControl(const CustomMessage::TimerControl& msg);
    void handleRecentsSave(const CustomMessage::TimerRecentsSave& msg);

    // -- Helpers --------------------------------------------------------------

    void sendStateToGui();
    void playEffect(Timer::Effect effect);
    void stopEffect();

    // -- TimerManager callbacks -----------------------------------------------

    void onFired(const Timer& timer) override;
    void onRecentsChanged(const std::vector<Timer>& list) override;
};

#endif // SERVICE_HPP
