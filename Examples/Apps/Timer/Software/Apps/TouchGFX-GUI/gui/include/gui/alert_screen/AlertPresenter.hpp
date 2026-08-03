#ifndef ALERTPRESENTER_HPP
#define ALERTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class AlertView;

/**
 * @brief Presenter for the Alert (effect) screen.
 *
 * Seeds the wheel from the model's edit timer and stores the chosen alert
 * effect back before the flow continues.
 */
class AlertPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    AlertPresenter(AlertView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~AlertPresenter() {}

    /** @brief Store the chosen alert effect on the model's edit timer. */
    void setEffect(Timer::Effect effect);

private:
    AlertPresenter();

    AlertView& view;
};

#endif // ALERTPRESENTER_HPP
