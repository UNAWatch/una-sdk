/**
 ******************************************************************************
 * @file    CountdownTimer.hpp
 * @brief   Shared widget: a self-registering countdown that fires a callback
 *          when it reaches zero, for screens that dismiss themselves.
 ******************************************************************************
 *
 * Inherits from touchgfx::Container solely to reach
 * Application::registerTimerWidget(). It draws nothing and does not belong in
 * the display tree.
 *
 * One start() yields one callback -- the countdown stops itself before calling
 * out. Calling start() again from inside the callback is how the repeating uses
 * (alarm replay, periodic re-indication) are built.
 *
 * Header-only and free of app-specific types, so every app shares this one copy
 * rather than carrying its own.
 *
 * Typical usage in a View:
 * @code
 *   // .hpp member:
 *   SDK::GUI::CountdownTimer      mDismissTimer;
 *   touchgfx::Callback<MyView>    mDismissCb;
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
 *
 ******************************************************************************
 */

#ifndef SDK_GUI_COUNTDOWN_TIMER_HPP
#define SDK_GUI_COUNTDOWN_TIMER_HPP

#include <cstdint>

#include <touchgfx/Application.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/containers/Container.hpp>

namespace SDK::GUI
{

/**
 * @brief A countdown that fires a callback once, for screens that dismiss
 *        themselves after a delay.
 *
 * Derives from touchgfx::Container only to get a per-frame tick:
 * Application::registerTimerWidget() takes a Drawable, and Drawable is abstract.
 * A bespoke Drawable subclass would work too and would save one pointer --
 * Container adds only firstChild -- but it would mean hand-writing draw(),
 * getSolidRect() and getLastChild(), which Container already answers correctly
 * for something with no children. Not worth four bytes.
 *
 * This widget has no children, no geometry and draws nothing; it does not belong
 * in the display tree.
 */
class CountdownTimer : public touchgfx::Container
{
public:
    CountdownTimer() = default;

    virtual ~CountdownTimer()
    {
        setRunning(false);
    }

    // The framework holds `this` while the timer runs. A copy would carry
    // mRunning without ever having been registered, and its destructor would
    // then unregister a pointer the framework never had.
    CountdownTimer(const CountdownTimer&) = delete;
    CountdownTimer& operator=(const CountdownTimer&) = delete;

    /**
     * @brief Set the countdown duration.
     * @param ticks Number of ticks (use SDK::Utils::secToTicks / SDK::Utils::msToTicks).
     */
    void setDuration(uint16_t ticks)
    {
        mDuration = ticks;
    }

    /**
     * @brief Register the callback invoked when the countdown expires.
     * @param cb Callback with no arguments.
     */
    void setCallback(touchgfx::GenericCallback<>& cb)
    {
        mpCb = &cb;
    }

    /** @brief Start, or restart, the countdown from the full duration. */
    void start()
    {
        mCounter = mDuration;
        setRunning(true);
    }

    /** @brief Stop the countdown. The callback will not fire. */
    void stop()
    {
        setRunning(false);
    }

    /**
     * @brief Push the deadline back to the full duration.
     *
     * Only meaningful while the countdown is running -- it deliberately leaves
     * the running state alone, so a stopped timer stays stopped. Use start() to
     * arm one again.
     */
    void reset()
    {
        mCounter = mDuration;
    }

    /** @brief True between start() and either stop() or the callback firing. */
    bool isRunning() const
    {
        return mRunning;
    }

    virtual void handleTickEvent() override
    {
        if (!mRunning) {
            return;
        }

        // A zero-duration countdown (mCounter already 0) expires on this tick
        // rather than staying armed forever; the guard also keeps the
        // decrement from underflowing the uint16.
        if (mCounter > 0 && --mCounter != 0) {
            return;
        }

        // Leave the running state before calling out, not after: isRunning() is
        // then false inside the callback, and a callback that calls start() to
        // repeat re-arms cleanly instead of being unregistered a moment later.
        setRunning(false);
        if (mpCb != nullptr && mpCb->isValid()) {
            mpCb->execute();
        }
    }

private:
    /**
     * @brief Enter or leave the running state.
     *
     * Registration with the framework and the flag that reports it change in
     * this one place, so they cannot drift apart.
     */
    void setRunning(bool running)
    {
        if (running == mRunning) {
            return;
        }

        mRunning = running;
        if (running) {
            touchgfx::Application::getInstance()->registerTimerWidget(this);
        } else {
            touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        }
    }

    uint16_t mDuration = 0;
    uint16_t mCounter  = 0;
    bool     mRunning  = false;

    touchgfx::GenericCallback<>* mpCb = nullptr;
};

} // namespace SDK::GUI

#endif // SDK_GUI_COUNTDOWN_TIMER_HPP
