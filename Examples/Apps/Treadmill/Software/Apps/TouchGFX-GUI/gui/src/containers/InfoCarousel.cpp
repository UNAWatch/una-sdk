#include <gui/containers/InfoCarousel.hpp>
#include <touchgfx/Application.hpp>

InfoCarousel::InfoCarousel()
{
}

InfoCarousel::~InfoCarousel()
{
    unregisterTimer();
}

void InfoCarousel::initialize()
{
    InfoCarouselBase::initialize();
}

// ---- Private helpers -------------------------------------------------------

void InfoCarousel::registerTimer()
{
    if (!mRegistered) {
        touchgfx::Application::getInstance()->registerTimerWidget(this);
        mRegistered = true;
    }
}

void InfoCarousel::unregisterTimer()
{
    if (mRegistered) {
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        mRegistered = false;
    }
}

void InfoCarousel::fireCallback()
{
    if (mpUpdateCb != nullptr && mpUpdateCb->isValid()) {
        mpUpdateCb->execute(static_cast<int16_t>(mIndex));
    }
}

// ---- Cycling control -------------------------------------------------------

void InfoCarousel::setCount(uint16_t count)
{
    mCount = count;

    if (mCount > 1) {
        registerTimer();
    } else {
        unregisterTimer();
    }

    mIndex = 0;
    mTick  = mPeriod;
    fireCallback();
}

void InfoCarousel::setPeriod(uint16_t ticks)
{
    mPeriod = (ticks > 0) ? ticks : 1;
    mTick   = mPeriod;
}

void InfoCarousel::setUpdateCallback(touchgfx::GenericCallback<int16_t>& cb)
{
    mpUpdateCb = &cb;
}

void InfoCarousel::refresh()
{
    fireCallback();
}

// ---- Content ---------------------------------------------------------------

void InfoCarousel::setTitle(TypedTextId msgId)
{
    Unicode::snprintf(titleTextBuffer, TITLETEXT_SIZE, "%s",
        touchgfx::TypedText(msgId).getText());
    titleText.invalidate();
}

void InfoCarousel::setValue(const touchgfx::Unicode::UnicodeChar* messageText)
{
    if (messageText != nullptr) {
        Unicode::snprintf(dataValueBuffer, DATAVALUE_SIZE, "%s", messageText);
        dataValue.setVisible(true);
    } else {
        dataValue.setVisible(false);
    }
    dataValue.invalidate();
}

// ---- Timer -----------------------------------------------------------------

void InfoCarousel::handleTickEvent()
{
    if (mTick > 0) {
        --mTick;
        return;
    }

    mTick  = mPeriod;
    mIndex = (mIndex + 1) % mCount;
    fireCallback();
}
