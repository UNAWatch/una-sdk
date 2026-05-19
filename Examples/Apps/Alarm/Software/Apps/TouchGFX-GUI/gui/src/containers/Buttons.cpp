#include <gui/containers/Buttons.hpp>
#include <touchgfx/Color.hpp>

Buttons::Buttons()
{
}

void Buttons::initialize()
{
    ButtonsBase::initialize();
}

// ---------------------------------------------------------------------------

touchgfx::colortype Buttons::toColortype(Color color)
{
    switch (color) {
        case WHITE: return touchgfx::Color::getColorFromRGB(192, 192, 192);
        case AMBER: return touchgfx::Color::getColorFromRGB(192, 128,   0);
        case RED:   return touchgfx::Color::getColorFromRGB(192,   0,   0);
        default:    return 0;
    }
}

void Buttons::applyColor(touchgfx::Circle& arc,
                         touchgfx::PainterABGR2222& painter,
                         Color color)
{
    arc.setVisible(color != NONE);
    if (color != NONE) {
        painter.setColor(toColortype(color));
    }
    arc.invalidate();
}

// ---------------------------------------------------------------------------

void Buttons::setL1(Color color) { applyColor(bL1, bL1Painter, color); }
void Buttons::setL2(Color color) { applyColor(bL2, bL2Painter, color); }
void Buttons::setR1(Color color) { applyColor(bR1, bR1Painter, color); }
void Buttons::setR2(Color color) { applyColor(bR2, bR2Painter, color); }
