#include <gui/containers/TimerRing.hpp>
#include <touchgfx/Application.hpp>

TimerRing::TimerRing()
{
}

void TimerRing::initialize()
{
    TimerRingBase::initialize();
    updateArc();
}

// -----------------------------------------------------------------------------
// handleTickEvent
// -----------------------------------------------------------------------------

void TimerRing::handleTickEvent()
{
    if (!mAnimating || mPaused) return;

    mAccumulator += mSpeed / mFPS;

    bool changed = false;
    while (mAccumulator >= 1.0f && mValue != mTarget)
    {
        mAccumulator -= 1.0f;
        mValue += (mValue < mTarget) ? 1 : -1;
        changed = true;
    }

    if (changed)
    {
        updateArc();
        if (mpValueChangedCb != nullptr && mpValueChangedCb->isValid())
        {
            mpValueChangedCb->execute(mValue);
        }
    }

    if (mValue == mTarget)
    {
        const int32_t finalValue = mValue;

        if (mLoop)
        {
            // Restart from the opposite end toward the original starting value
            const int32_t newTarget = (finalValue == mMaxValue) ? 0 : mMaxValue;
            mAccumulator = 0.0f;
            mTarget = newTarget;
        }
        else
        {
            stop();
        }

        if (mpCompleteCb != nullptr && mpCompleteCb->isValid())
        {
            mpCompleteCb->execute(finalValue);
        }
    }
}

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

void TimerRing::setFPS(float fps)
{
    mFPS = (fps > 0.0f) ? fps : 1.0f;
}

void TimerRing::setMaxValue(int32_t max)
{
    mMaxValue = (max > 0) ? max : 1;
    updateArc();
}

void TimerRing::setSpeed(float unitsPerSecond)
{
    mSpeed = (unitsPerSecond > 0.0f) ? unitsPerSecond : 1.0f;
}

void TimerRing::setMode(Mode mode)
{
    mMode = mode;
    updateArc();
}

void TimerRing::setProgressColor(touchgfx::colortype color)
{
    progressPainter.setColor(color);
    progress.invalidate();
}

void TimerRing::setTrackColor(touchgfx::colortype color)
{
    trackPainter.setColor(color);
    track.invalidate();
}

void TimerRing::setLoopEnabled(bool enabled)
{
    mLoop = enabled;
}

// -----------------------------------------------------------------------------
// Control
// -----------------------------------------------------------------------------

void TimerRing::setValue(int32_t value)
{
    stop();
    mValue = (value < 0) ? 0 : (value > mMaxValue ? mMaxValue : value);
    mTarget = mValue;
    mAccumulator = 0.0f;
    updateArc();
}

void TimerRing::animateTo(int32_t target)
{
    mTarget = (target < 0) ? 0 : (target > mMaxValue ? mMaxValue : target);
    if (mValue == mTarget) return;

    mAccumulator = 0.0f;
    mPaused = false;

    if (!mAnimating)
    {
        touchgfx::Application::getInstance()->registerTimerWidget(this);
        mAnimating = true;
    }
}

void TimerRing::pause()
{
    mPaused = true;
}

void TimerRing::resume()
{
    if (mAnimating) mPaused = false;
}

void TimerRing::stop()
{
    if (mAnimating)
    {
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        mAnimating = false;
    }
    mPaused = false;
    mAccumulator = 0.0f;
}

// -----------------------------------------------------------------------------
// Getters
// -----------------------------------------------------------------------------

float TimerRing::getProgress() const
{
    return (mMaxValue > 0) ? float(mValue) / float(mMaxValue) : 0.0f;
}

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void TimerRing::updateArc()
{
    const int16_t trackStart = track.getArcStart();
    const int16_t trackEnd   = track.getArcEnd();
    const float   span       = float(trackEnd - trackStart);
    const int16_t arcSpan    = int16_t(span * getProgress() + 0.5f);

    if (mMode == FILL)
    {
        progress.setArc(trackStart, trackStart + arcSpan);
    }
    else // DRAIN
    {
        progress.setArc(trackEnd - arcSpan, trackEnd);
    }

    progress.invalidate();
    track.invalidate();
}
