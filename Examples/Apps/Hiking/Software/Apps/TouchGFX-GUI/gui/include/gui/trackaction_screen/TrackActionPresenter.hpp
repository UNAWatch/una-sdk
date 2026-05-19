#ifndef TRACKACTIONPRESENTER_HPP
#define TRACKACTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackActionView;

class TrackActionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackActionPresenter(TrackActionView& v);

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

    virtual ~TrackActionPresenter() {}

    virtual void onTrackData(const Track::Data& data) override;

    void resumeTrack();

private:
    TrackActionPresenter();

    TrackActionView& view;
};

#endif // TRACKACTIONPRESENTER_HPP
