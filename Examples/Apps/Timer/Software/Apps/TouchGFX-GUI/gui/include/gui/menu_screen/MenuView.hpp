#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/MenuViewBase.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>

/**
 * @brief Menu screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class MenuView : public MenuViewBase
{
public:
    MenuView() {}
    virtual ~MenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // MENUVIEW_HPP
