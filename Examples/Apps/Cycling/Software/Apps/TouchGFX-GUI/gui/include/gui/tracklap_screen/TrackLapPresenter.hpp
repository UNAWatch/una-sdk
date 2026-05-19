#ifndef TRACKLAPPRESENTER_HPP
#define TRACKLAPPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackLapView;

class TrackLapPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackLapPresenter(TrackLapView& v);

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

    virtual ~TrackLapPresenter() {}

private:
    TrackLapPresenter();

    TrackLapView& view;
};

#endif // TRACKLAPPRESENTER_HPP
