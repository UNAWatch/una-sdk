#include <gui/containers/ScrollIndicator.hpp>
#include <touchgfx/Application.hpp>


// ---------------------------------------------------------------------------

ScrollIndicator::ScrollIndicator()
{
}

ScrollIndicator::~ScrollIndicator()
{
    stopAnimation();
}

void ScrollIndicator::initialize()
{
    ScrollIndicatorBase::initialize();
    // Designer-generated base already applies kBig geometry and colours
    // in its constructor, so no explicit setConfig() call is needed here.
    // It also draws the handle at the top of the rail; mirror that, so an
    // animation started before the first setCount() has the right origin.
    mHandlePos = mRailMax - mHandleLen;
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void ScrollIndicator::setConfig(const ScrollIndicatorConfig& cfg)
{
    mHandleLen = cfg.handleLen;
    mRailMin   = cfg.railMin;
    mRailMax   = cfg.railMax;

    rail.setArc(cfg.railMin, cfg.railMax);
    railPainter.setColor(cfg.railColor);
    rail.invalidate();

    handlePainter.setColor(cfg.handleColor);
    handleOvfPainter.setColor(cfg.handleColor);

    // Re-draw handle at the current position with the new geometry
    setActiveId(mPosition);
}

// ---------------------------------------------------------------------------
// Item navigation
// ---------------------------------------------------------------------------

void ScrollIndicator::setCount(uint16_t count)
{
    if (count == 0) {
        count = 1;
    }

    mCount = count;
    handle.setVisible(mCount > 1);
    handle.invalidate();

    setActiveId(0);
}

void ScrollIndicator::stopAnimation()
{
    if (mAnimationRunning) {
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        mAnimationRunning = false;
    }

    // Always clean up the overflow handle -- it may be visible from a
    // wrap-around animation that was interrupted before it could hide itself.
    if (mAnimWrap) {
        handleOvf.setVisible(false);
        handleOvf.invalidate();
        mAnimWrap = false;
    }
}

void ScrollIndicator::setActiveId(uint16_t index)
{
    stopAnimation();  // ensure handleOvf is hidden and timer unregistered

    if (mCount <= 1) {
        index = 0;
    }

    mPosition = index;
    const float start = getStartAngle(index);

    mHandlePos = start;
    setClampedArc(handle, start, mCount > 1);
    rail.invalidate();
}

void ScrollIndicator::setClampedArc(touchgfx::Circle& arc, float start, bool baseVisible)
{
    float a = start;
    float b = start + mHandleLen;
    a = (a < mRailMin) ? mRailMin : ((a > mRailMax) ? mRailMax : a);
    b = (b < mRailMin) ? mRailMin : ((b > mRailMax) ? mRailMax : b);

    const bool visible = baseVisible && (b > a);
    arc.setVisible(visible);
    if (visible) {
        arc.setArc(a, b);
    }
    arc.invalidate();
}

void ScrollIndicator::animateToId(int16_t index, int16_t animationSteps)
{
    if (mCount <= 1) {
        index = 0;
    }

    bool wrapForward = false;
    bool isWrap      = false;

    // Clamp and flag wrap-around for the dual-handle overflow animation
    if (index < 0) {
        index  = static_cast<int16_t>(mCount) - 1;
        isWrap = true;
    } else if (index >= static_cast<int16_t>(mCount)) {
        index       = 0;
        isWrap      = true;
        wrapForward = true;
    }

    if (animationSteps <= 0) {
        stopAnimation();
        setActiveId(static_cast<uint16_t>(index));
        return;
    }

    // Handle the case where a previous animation is still running
    if (mAnimationRunning) {
        if (mAnimWrap) {
            // In a wrap-around animation the overflow handle is the one that
            // visually approaches the target. Hand its position to the main
            // handle so the next animation starts from the right spot.
            mHandlePos = mOvfPos;
            setClampedArc(handle, mHandlePos, mCount > 1);
            handleOvf.setVisible(false);
            handleOvf.invalidate();
            mAnimWrap = false;
        }
        // Timer is already registered -- no need to re-register.
    } else {
        touchgfx::Application::getInstance()->registerTimerWidget(this);
    }

    mAnimWrap          = isWrap;
    mAnimationDuration = static_cast<uint16_t>(animationSteps);
    mAnimationCounter  = 0;
    mPosition          = static_cast<uint16_t>(index);

    // Start from the handle's current position (may be mid-animation)
    mAnimationStartPos = mHandlePos;
    mAnimationEndPos = getStartAngle(static_cast<uint16_t>(index));

    if (isWrap) {
        // Outgoing handle slides off one end of the rail,
        // incoming handle slides in from the opposite end.
        if (wrapForward) {
            mAnimOutgoingEnd   = mAnimationStartPos - mHandleLen;
            mAnimIncomingStart = mAnimationEndPos   + mHandleLen;
        } else {
            mAnimOutgoingEnd   = mAnimationStartPos + mHandleLen;
            mAnimIncomingStart = mAnimationEndPos   - mHandleLen;
        }

        // Defined from the moment the wrap is armed, so an interrupt landing
        // before the first tick still reads a real position.
        mOvfPos = mAnimIncomingStart;
    }

    mAnimationRunning = true;
}

uint16_t ScrollIndicator::getActiveId()
{
    return mPosition;
}

// ---------------------------------------------------------------------------
// Tick / animation
// ---------------------------------------------------------------------------

void ScrollIndicator::handleTickEvent()
{
    nextAnimationStep();
}

void ScrollIndicator::nextAnimationStep()
{
    if (!mAnimationRunning) {
        return;
    }

    mAnimationCounter++;

    const float t = static_cast<float>(mAnimationCounter) / mAnimationDuration;

    float pos;
    if (mAnimWrap) {
        // Outgoing handle slides toward the rail edge
        pos = mAnimationStartPos + t * (mAnimOutgoingEnd - mAnimationStartPos);

        // Incoming handle appears from the opposite rail edge (clipped to the
        // rail so it grows out of the edge rather than floating past it).
        const float incomingPos = mAnimIncomingStart + t * (mAnimationEndPos - mAnimIncomingStart);
        mOvfPos = incomingPos;
        setClampedArc(handleOvf, incomingPos, true);
    } else {
        pos = mAnimationStartPos + t * (mAnimationEndPos - mAnimationStartPos);
    }

    mHandlePos = pos;
    setClampedArc(handle, pos, mCount > 1);
    rail.invalidate();

    if (mAnimationCounter >= mAnimationDuration) {
        mAnimationRunning = false;
        mAnimationCounter = 0;
        touchgfx::Application::getInstance()->unregisterTimerWidget(this);

        if (mAnimWrap) {
            mAnimWrap = false;
            handleOvf.setVisible(false);
            handleOvf.invalidate();
            setActiveId(mPosition);
        }
    }
}

// ---------------------------------------------------------------------------

float ScrollIndicator::getStartAngle(uint16_t index) const
{
    if (mCount <= 1) {
        // Single item: center the handle on the rail
        return (mRailMax + mRailMin - mHandleLen) / 2.0f;
    }

    const float step   = (mRailMax - mRailMin - mHandleLen) / (mCount - 1);
    const float offset = step * index;
    return (mRailMax - mHandleLen) - offset;
}
