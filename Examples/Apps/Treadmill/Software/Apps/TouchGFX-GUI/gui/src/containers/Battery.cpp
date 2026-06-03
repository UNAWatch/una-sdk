#include <gui/containers/Battery.hpp>

// -----------------------------------------------------------------------------
// Segment colors
// -----------------------------------------------------------------------------
static constexpr uint32_t kColorInactive = 0x404040u;
static constexpr uint32_t kColorActive   = 0x008080u;
static constexpr uint32_t kColorLow      = 0xC00000u;

// -----------------------------------------------------------------------------
Battery::Battery()
{
}

void Battery::initialize()
{
    BatteryBase::initialize();
}

// -----------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------

void Battery::setSegment(touchgfx::Box& box,
                         touchgfx::Line& line1, touchgfx::PainterABGR2222& p1,
                         touchgfx::Line& line2, touchgfx::PainterABGR2222& p2,
                         touchgfx::colortype color)
{
    box.setColor(color);
    p1.setColor(color);
    p2.setColor(color);
    box.invalidate();
    line1.invalidate();
    line2.invalidate();
}

void Battery::setSegment4(touchgfx::colortype color)
{
    s4box.setColor(color);
    s4line1Painter.setColor(color);
    s4line2Painter.setColor(color);
    s4line3Painter.setColor(color);
    s4box.invalidate();
    s4line1.invalidate();
    s4line2.invalidate();
    s4line3.invalidate();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void Battery::setLevel(uint8_t level)
{
    // s1: red at 1-24 %, inactive at 0 %
    const touchgfx::colortype c1 = (level == 0)  ? kColorInactive :
                                   (level  < 25) ? kColorLow      : kColorActive;
    setSegment(s1box, s1line1, s1line1Painter, s1line2, s1line2Painter, c1);

    setSegment(s2box, s2line1, s2line1Painter, s2line2, s2line2Painter,
               level >= 25 ? kColorActive : kColorInactive);

    setSegment(s3box, s3line1, s3line1Painter, s3line2, s3line2Painter,
               level >= 50 ? kColorActive : kColorInactive);

    setSegment4(level >= 75 ? kColorActive : kColorInactive);
}
