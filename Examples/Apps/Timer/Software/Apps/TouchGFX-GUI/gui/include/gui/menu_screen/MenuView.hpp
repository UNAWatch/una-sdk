#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/MenuViewBase.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Menu screen: act on the chosen timer (Start / Edit / Delete).
 *
 * An OrbitMenu lists the three actions, but the container is positioned so the
 * row above the selection is clipped away by its top edge -- only the centre
 * (on a teal pill) and the row below are visible. The freed top slot instead
 * holds a static teal label with the timer's value (MM:SS). L1/L2 scroll, R1
 * runs the centred action, R2 returns to Main.
 */
class MenuView : public MenuViewBase
{
public:
    MenuView();
    virtual ~MenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Show the timer value acted on, formatted MM:SS. */
    void setValue(uint16_t durationSec);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    void confirm();
    void onOrbitAnimationEnded();
    /**
     * @brief Refresh Start-only accents: the play glyph and a teal R1 arc when
     * Start is centred, hidden glyph and a white R1 otherwise.
     */
    void updateStartVisuals();

    static const int16_t kCount = 3;   ///< Start / Edit / Delete.

    touchgfx::Callback<MenuView> mAnimEndedCb;
};

#endif // MENUVIEW_HPP
