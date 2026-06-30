#ifndef TRACKHOLDCONFIRMATIONPRESENTER_HPP
#define TRACKHOLDCONFIRMATIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackHoldConfirmationView;

class TrackHoldConfirmationPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackHoldConfirmationPresenter(TrackHoldConfirmationView& v);

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

    virtual ~TrackHoldConfirmationPresenter() {}

    /** Which action the hold screen should confirm (set by the action menu). */
    Model::HoldConfirmMode getHoldConfirmMode() const { return model->getHoldConfirmMode(); }

    virtual void onIdleTimeout() override { model->application().gotoTrackActionScreenNoTransition(); }

private:
    TrackHoldConfirmationPresenter();

    TrackHoldConfirmationView& view;
};

#endif // TRACKHOLDCONFIRMATIONPRESENTER_HPP
