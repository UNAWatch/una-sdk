#ifndef TRACKINTERVALSCOUNTDOWNPRESENTER_HPP
#define TRACKINTERVALSCOUNTDOWNPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackIntervalsCountdownView;

class TrackIntervalsCountdownPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackIntervalsCountdownPresenter(TrackIntervalsCountdownView& v);

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

    virtual ~TrackIntervalsCountdownPresenter() {}

    void startTrack();

private:
    TrackIntervalsCountdownPresenter();

    TrackIntervalsCountdownView& view;
};

#endif // TRACKINTERVALSCOUNTDOWNPRESENTER_HPP
