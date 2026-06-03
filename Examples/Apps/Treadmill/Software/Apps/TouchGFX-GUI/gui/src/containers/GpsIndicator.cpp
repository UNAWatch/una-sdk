#include <gui/containers/GpsIndicator.hpp>
#include <touchgfx/Application.hpp>

GpsIndicator::GpsIndicator()
{

}

GpsIndicator::~GpsIndicator()
{
    unregisterTimer();
}

void GpsIndicator::initialize()
{
    GpsIndicatorBase::initialize();
    setVisible(false);  // start from the OFF phase -- no phantom flash on screen re-entry
    registerTimer();
}

void GpsIndicator::setPeriod(uint32_t ticks)
{
    mPeriod = ticks;
}

void GpsIndicator::setAcquired(bool acquired)
{
    mAcquired = acquired;
    if (!acquired) {
        registerTimer();  // GPS lost -- resume blinking
    }
    // When acquired: timer keeps running; handleTickEvent hides and unregisters.
}

void GpsIndicator::handleTickEvent()
{
    // Already hidden after acquiring fix -- nothing left to do.
    if (mAcquired && !isVisible()) {
        unregisterTimer();
        mCounter = 0;
        return;
    }

    if (++mCounter < mPeriod) {
        return;
    }
    mCounter = 0;

    if (mAcquired) {
        setVisible(false);
    } else {
        setVisible(!isVisible());
    }
    invalidate();
}

void GpsIndicator::registerTimer()
{
    if (!mRegistered) {
        touchgfx::Application::getInstance()->registerTimerWidget(this);
        mRegistered = true;
    }
}

void GpsIndicator::unregisterTimer()
{
    if (mRegistered) {
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        mRegistered = false;
    }
}
