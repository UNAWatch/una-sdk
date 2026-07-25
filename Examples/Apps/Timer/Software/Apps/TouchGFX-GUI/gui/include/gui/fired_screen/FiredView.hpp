#ifndef FIREDVIEW_HPP
#define FIREDVIEW_HPP

#include <gui_generated/fired_screen/FiredViewBase.hpp>
#include <gui/fired_screen/FiredPresenter.hpp>

/**
 * @brief Fired screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class FiredView : public FiredViewBase
{
public:
    FiredView() {}
    virtual ~FiredView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // FIREDVIEW_HPP
