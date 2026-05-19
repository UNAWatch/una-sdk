#ifndef TRACKDISCARDEDPRESENTER_HPP
#define TRACKDISCARDEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackDiscardedView;

class TrackDiscardedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackDiscardedPresenter(TrackDiscardedView& v);

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

    virtual ~TrackDiscardedPresenter() {}

    void exitApp();

private:
    TrackDiscardedPresenter();

    TrackDiscardedView& view;
};

#endif // TRACKDISCARDEDPRESENTER_HPP
