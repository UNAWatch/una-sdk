#include <gui/tracksummary_screen/TrackSummaryView.hpp>
#include <gui/tracksummary_screen/TrackSummaryPresenter.hpp>

TrackSummaryPresenter::TrackSummaryPresenter(TrackSummaryView& v)
    : view(v)
{
}

void TrackSummaryPresenter::activate()
{
    model->resetIdleTimer();

    view.setSummary(model->getTrackSummary(), model->isTrackPaused());
}

void TrackSummaryPresenter::deactivate()
{
}

void TrackSummaryPresenter::exitApp()
{
    model->exitApp();
}

void TrackSummaryPresenter::backToTrack()
{
    if (model->isTrackPaused()) {
        model->application().gotoTrackActionScreenNoTransition();
    } else {
        model->exitApp();
    }
}
