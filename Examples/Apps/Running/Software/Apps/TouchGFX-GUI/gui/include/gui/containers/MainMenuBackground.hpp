#ifndef MAINMENUBACKGROUND_HPP
#define MAINMENUBACKGROUND_HPP

#include <gui_generated/containers/MainMenuBackgroundBase.hpp>

/**
 * @brief Background for the main menu screen.
 *
 * Wraps a single canvas Circle that fills the region under center menu item.
 * The fill color can be changed at any time via setColor().
 */
class MainMenuBackground : public MainMenuBackgroundBase
{
public:
    MainMenuBackground();
    virtual ~MainMenuBackground() {}

    virtual void initialize();

    /**
     * @brief Set the background fill color.
     * @param color Desired fill color.
     */
    void setColor(touchgfx::colortype color);

protected:
};

#endif // MAINMENUBACKGROUND_HPP
