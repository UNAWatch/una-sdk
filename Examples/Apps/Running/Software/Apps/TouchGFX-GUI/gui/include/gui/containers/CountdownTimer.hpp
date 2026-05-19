#ifndef COUNTDOWNTIMER_HPP
#define COUNTDOWNTIMER_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Self-registering countdown timer for auto-dismissing screens.
 *
 * Inherits from touchgfx::Container solely to gain access to
 * Application::registerTimerWidget(). Does not render anything and does
 * not need to be added to the display tree.
 *
 * The callback is fired once when the countdown reaches zero.
 * After firing, the timer stops automatically.
 *
 * Typical usage in a View:
 * @code
 *   // .hpp member:
 *   CountdownTimer                        mDismissTimer;
 *   touchgfx::Callback<MyView>            mDismissCb;
 *
 *   // constructor:
 *   MyView() : mDismissCb(this, &MyView::onDismiss) {}
 *
 *   // setupScreen():
 *   mDismissTimer.setDuration(SDK::Utils::secToTicks(3, 10));
 *   mDismissTimer.setCallback(mDismissCb);
 *   mDismissTimer.start();
 *
 *   // callback:
 *   void onDismiss() { application().gotoXxxScreenNoTransition(); }
 *
 *   // handleKeyEvent (optional -- manual dismiss):
 *   mDismissTimer.stop();
 *   application().gotoXxxScreenNoTransition();
 * @endcode
 */
class CountdownTimer : public touchgfx::Container
{
public:
    CountdownTimer();
    virtual ~CountdownTimer();

    /**
     * @brief Set the countdown duration.
     * @param ticks Number of ticks (use SDK::Utils::secToTicks / SDK::Utils::msToTicks).
     */
    void setDuration(uint16_t ticks);

    /**
     * @brief Register the callback invoked when the countdown expires.
     * @param cb Callback with no arguments.
     */
    void setCallback(touchgfx::GenericCallback<>& cb);

    /** @brief Start (or restart) the countdown and register the timer widget. */
    void start();

    /** @brief Stop the countdown and unregister the timer widget. */
    void stop();

    /** @brief Restart the countdown from the full duration without unregistering. */
    void reset();

    virtual void handleTickEvent() override;

private:
    void registerTimer();
    void unregisterTimer();

    uint16_t mDuration   = 0;
    uint16_t mCounter    = 0;
    bool     mRegistered = false;

    touchgfx::GenericCallback<>* mpCb = nullptr;
};

#endif // COUNTDOWNTIMER_HPP
