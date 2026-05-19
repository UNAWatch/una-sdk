#ifndef TRACKDISCARDCONFIRMATIONPRESENTER_HPP
#define TRACKDISCARDCONFIRMATIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackDiscardConfirmationView;

class TrackDiscardConfirmationPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackDiscardConfirmationPresenter(TrackDiscardConfirmationView& v);

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

    virtual ~TrackDiscardConfirmationPresenter() {}

    virtual void onIdleTimeout() override { model->application().gotoTrackActionScreenNoTransition(); }

private:
    TrackDiscardConfirmationPresenter();

    TrackDiscardConfirmationView& view;
};

#endif // TRACKDISCARDCONFIRMATIONPRESENTER_HPP
