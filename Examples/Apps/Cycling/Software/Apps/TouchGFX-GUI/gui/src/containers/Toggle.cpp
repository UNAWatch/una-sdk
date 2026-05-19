#include <gui/containers/Toggle.hpp>

// Default colors -- must match the values set in ToggleBase constructor.
// PainterABGR2222 has no getColor(), so they cannot be read back at runtime.
static constexpr uint32_t kDefaultRailOff     = 0x000000u;
static constexpr uint32_t kDefaultRailOn      = 0xC08000u;
static constexpr uint32_t kDefaultHandleColor = 0xC0C0C0u;

// -----------------------------------------------------------------------------

Toggle::Toggle()
    : mRailOnColor(kDefaultRailOn)
    , mRailOffColor(kDefaultRailOff)
    , mHandleOnColor(kDefaultHandleColor)
    , mHandleOffColor(kDefaultHandleColor)
{
}

void Toggle::initialize()
{
    ToggleBase::initialize();
}

// -----------------------------------------------------------------------------

void Toggle::setState(bool state)
{
    mState = state;

    railPainter.setColor(mState ? mRailOnColor : mRailOffColor);
    rail.invalidate();

    handlePainter.setColor(mState ? mHandleOnColor : mHandleOffColor);
    handle.setX(mState ? kHandleOnX : kHandleOffX);
    handle.invalidate();
}

bool Toggle::getState() const
{
    return mState;
}

// -----------------------------------------------------------------------------

void Toggle::setRailOnColor(touchgfx::colortype color)
{
    mRailOnColor = color;
}

void Toggle::setRailOffColor(touchgfx::colortype color)
{
    mRailOffColor = color;
}

void Toggle::setHandleOnColor(touchgfx::colortype color)
{
    mHandleOnColor = color;
}

void Toggle::setHandleOffColor(touchgfx::colortype color)
{
    mHandleOffColor = color;
}
