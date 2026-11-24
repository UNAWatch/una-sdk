#ifndef TRACKRUNALERTPRESENTER_HPP
#define TRACKRUNALERTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackRunAlertView;

class TrackRunAlertPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackRunAlertPresenter(TrackRunAlertView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~TrackRunAlertPresenter() {}

private:
    TrackRunAlertPresenter();

    TrackRunAlertView& view;
};

#endif // TRACKRUNALERTPRESENTER_HPP
