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

    /// Another alert while this one shows: show it instead, for the full time.
    virtual void onRunAlert() override;

    virtual ~TrackRunAlertPresenter() {}

private:
    TrackRunAlertPresenter();

    void showAlert();

    TrackRunAlertView& view;
};

#endif // TRACKRUNALERTPRESENTER_HPP
