#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include <gui_generated/containers/ButtonsBase.hpp>

/**
 * @brief Physical button indicator container.
 *
 * Displays four watch-bezel arc indicators (L1, L2, R1, R2), each in one of
 * four visual states: hidden (NONE), white, amber, or red.
 *
 * The arc geometry is fixed by the Designer-generated base. Only the colour
 * and visibility are controlled at runtime.
 *
 * Usage:
 * @code
 *   buttons.setL1(Buttons::NONE);
 *   buttons.setR1(Buttons::AMBER);
 *   buttons.setR2(Buttons::WHITE);
 * @endcode
 */
class Buttons : public ButtonsBase
{
public:
    Buttons();
    virtual ~Buttons() {}

    virtual void initialize();

    /** @brief Visual state for a single button indicator. */
    enum Color
    {
        NONE  = 0, ///< Indicator hidden
        WHITE,     ///< White  -- neutral action  (#C0C0C0)
        AMBER,     ///< Amber  -- primary action  (#C08000)
        RED,       ///< Red    -- warning / destructive action  (#C00000)
    };

    void setL1(Color color = NONE); ///< Top-left button
    void setL2(Color color = NONE); ///< Bottom-left button
    void setR1(Color color = NONE); ///< Top-right button
    void setR2(Color color = NONE); ///< Bottom-right button

private:
    /** Map a Color enum value to its RGB colortype. */
    static touchgfx::colortype toColortype(Color color);

    /** Apply @p color to a single arc indicator. */
    void applyColor(touchgfx::Circle& arc,
                    touchgfx::PainterABGR2222& painter,
                    Color color);
};

#endif // BUTTONS_HPP
