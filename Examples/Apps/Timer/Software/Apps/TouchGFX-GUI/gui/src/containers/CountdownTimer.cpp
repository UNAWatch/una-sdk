#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Application.hpp>

CountdownTimer::CountdownTimer()
{
}

CountdownTimer::~CountdownTimer()
{
    unregisterTimer();
}

// ---- Configuration ---------------------------------------------------------

void CountdownTimer::setDuration(uint16_t ticks)
{
    mDuration = ticks;
}

void CountdownTimer::setCallback(touchgfx::GenericCallback<>& cb)
{
    mpCb = &cb;
}

// ---- Control ---------------------------------------------------------------

void CountdownTimer::start()
{
    mCounter = mDuration;
    registerTimer();
}

void CountdownTimer::stop()
{
    unregisterTimer();
}

void CountdownTimer::reset()
{
    mCounter = mDuration;
    // Timer stays registered -- no unregister/register cycle needed.
}

// ---- Tick ------------------------------------------------------------------

void CountdownTimer::handleTickEvent()
{
    if (!mRegistered) {
        return;
    }

    // A zero-duration countdown (mCounter already 0) expires on this tick rather
    // than staying registered forever; the guard also avoids a uint16 underflow.
    if (mCounter > 0 && --mCounter != 0) {
        return;
    }

    unregisterTimer();
    if (mpCb != nullptr && mpCb->isValid()) {
        mpCb->execute();
    }
}

// ---- Private ---------------------------------------------------------------

void CountdownTimer::registerTimer()
{
    if (!mRegistered) {
        touchgfx::Application::getInstance()->registerTimerWidget(this);
        mRegistered = true;
    }
}

void CountdownTimer::unregisterTimer()
{
    if (mRegistered) {
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        mRegistered = false;
    }
}
