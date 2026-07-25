#ifndef TOGGLE_HPP
#define TOGGLE_HPP

#include <gui_generated/containers/ToggleBase.hpp>

/**
 * @brief Two-state toggle switch widget.
 *
 * The rail (Line) changes color and the handle (Circle) slides
 * between two positions to indicate ON / OFF state.
 *
 * Default colors:
 *  - Rail  OFF : #000000 (black)
 *  - Rail  ON  : #C08000 (amber)
 *  - Handle    : unchanged
 *
 * Colors can be customised at runtime via setRailOnColor() etc.
 * before the first setState() call.
 *
 * Usage:
 * @code
 *   toggle.setState(true);            // switch ON with default colors
 *   toggle.setRailOnColor(myColor);   // override ON rail color
 *   bool on = toggle.getState();
 * @endcode
 */
class Toggle : public ToggleBase
{
public:
    Toggle();
    virtual ~Toggle() {}

    virtual void initialize();

    /** @brief Set the ON/OFF state and update visuals immediately. */
    void setState(bool state);

    /** @brief Return the current ON/OFF state. */
    bool getState() const;

    /** @brief Set the rail color used when the toggle is ON. */
    void setRailOnColor(touchgfx::colortype color);

    /** @brief Set the rail color used when the toggle is OFF. */
    void setRailOffColor(touchgfx::colortype color);

    /** @brief Set the handle color used when the toggle is ON. */
    void setHandleOnColor(touchgfx::colortype color);

    /** @brief Set the handle color used when the toggle is OFF. */
    void setHandleOffColor(touchgfx::colortype color);

protected:
    static constexpr int16_t kHandleOnX  = 30; ///< Handle X position when ON
    static constexpr int16_t kHandleOffX =  0; ///< Handle X position when OFF

    bool mState = false; ///< Current ON/OFF state

    touchgfx::colortype mRailOnColor;
    touchgfx::colortype mRailOffColor;
    touchgfx::colortype mHandleOnColor;
    touchgfx::colortype mHandleOffColor;
};

#endif // TOGGLE_HPP
