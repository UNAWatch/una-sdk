#ifndef TRACKSAVEDPRESENTER_HPP
#define TRACKSAVEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackSavedView;

class TrackSavedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackSavedPresenter(TrackSavedView& v);

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

    virtual ~TrackSavedPresenter() {}

private:
    TrackSavedPresenter();

    TrackSavedView& view;
};

#endif // TRACKSAVEDPRESENTER_HPP
