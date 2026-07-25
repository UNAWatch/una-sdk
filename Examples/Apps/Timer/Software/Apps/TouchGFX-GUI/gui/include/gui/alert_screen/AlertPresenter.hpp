#ifndef ALERTPRESENTER_HPP
#define ALERTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class AlertView;

/**
 * @brief Presenter for the Alert screen -- skeleton placeholder.
 */
class AlertPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    AlertPresenter(AlertView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~AlertPresenter() {}

private:
    AlertPresenter();

    AlertView& view;
};

#endif // ALERTPRESENTER_HPP
