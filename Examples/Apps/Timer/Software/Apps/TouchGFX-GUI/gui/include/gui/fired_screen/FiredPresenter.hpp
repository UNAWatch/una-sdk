#ifndef FIREDPRESENTER_HPP
#define FIREDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class FiredView;

/**
 * @brief Presenter for the Fired screen -- skeleton placeholder.
 */
class FiredPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    FiredPresenter(FiredView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~FiredPresenter() {}

private:
    FiredPresenter();

    FiredView& view;
};

#endif // FIREDPRESENTER_HPP
