#include <gui/tracklap_screen/TrackLapView.hpp>
#include <gui/tracklap_screen/TrackLapPresenter.hpp>

TrackLapPresenter::TrackLapPresenter(TrackLapView& v)
    : view(v)
{
}

void TrackLapPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();

    view.setUnitsImperial(model->isUnitsImperial());

    const Track::Data& data = model->getTrackData();

    view.setLapNum(data.lapNum);
    view.setDistance(data.lapDistance);
    view.setTimer(data.lapTime);
    view.setPace(data.lapPace);
}

void TrackLapPresenter::deactivate()
{
}
