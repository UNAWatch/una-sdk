#ifndef TRACKSUMMARYPRESENTER_HPP
#define TRACKSUMMARYPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackSummaryView;

class TrackSummaryPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackSummaryPresenter(TrackSummaryView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~TrackSummaryPresenter() {}

    void exitApp();

    /**
     * @brief Called when R1 is pressed.
     * Returns to TrackAction screen if track is paused, otherwise exits the app.
     */
    void backToTrack();

private:
    TrackSummaryPresenter();

    TrackSummaryView& view;
};

#endif // TRACKSUMMARYPRESENTER_HPP
