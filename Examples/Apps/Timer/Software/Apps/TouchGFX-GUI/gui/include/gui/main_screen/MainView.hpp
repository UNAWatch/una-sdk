#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @brief Main screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class MainView : public MainViewBase
{
public:
    MainView() {}
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // MAINVIEW_HPP
