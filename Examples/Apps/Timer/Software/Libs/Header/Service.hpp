#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/HomeWidget/HomeWidget.hpp"

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
    SDK::HomeWidget       mWidget;

    // A fire that happened while the GUI was closed: launch the GUI, then
    // deliver it once the GUI signals it is running. mPendingFiredTick bounds the
    // wait so a dropped launch (GUI never signals) cannot pin the service alive.
    bool                  mPendingFired = false;
    Timer                 mPendingTimer {};
    uint32_t              mPendingFiredTick = 0;

    // Home-screen widget: shown while a countdown is RUNNING/PAUSED *and the GUI
    // is closed* -- with the app open the user already sees the Running screen.
    bool                  mWidgetActive  = false;  ///< Widget currently claimed.
    int32_t               mLastWidgetSec = -1;     ///< Last MM:SS pushed (dedup).

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

    /**
     * @brief Push the home widget from the current countdown state.
     *
     * Claims/releases the widget as the countdown (with the GUI closed) enters/
     * leaves RUNNING/PAUSED, refreshes MM:SS + completion percent when the shown
     * second changes, and caps @p sleepTime so the loop wakes on the next second
     * boundary while running (a paused countdown needs no periodic wake).
     */
    void pumpWidget(uint32_t now, uint32_t& sleepTime);

    // -- TimerManager callbacks -----------------------------------------------

    void onFired(const Timer& timer) override;
    void onRecentsChanged(const std::vector<Timer>& list) override;
};

#endif // SERVICE_HPP
