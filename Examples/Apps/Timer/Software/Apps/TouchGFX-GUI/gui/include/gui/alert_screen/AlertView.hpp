#ifndef ALERTVIEW_HPP
#define ALERTVIEW_HPP

#include <gui_generated/alert_screen/AlertViewBase.hpp>
#include <gui/alert_screen/AlertPresenter.hpp>

/**
 * @brief Alert screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class AlertView : public AlertViewBase
{
public:
    AlertView() {}
    virtual ~AlertView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // ALERTVIEW_HPP
