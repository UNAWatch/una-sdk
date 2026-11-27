#include <gui/tracklap_screen/TrackLapView.hpp>
#include <gui/tracklap_screen/TrackLapPresenter.hpp>

TrackLapPresenter::TrackLapPresenter(TrackLapView& v)
    : view(v)
{
}

void TrackLapPresenter::activate()
{
    showLap();
}

void TrackLapPresenter::onLapChanged(uint8_t /*lapEnd*/)
{
    showLap();
    view.restartDismiss();
}

void TrackLapPresenter::showLap()
{
    model->takePopup(Model::Popup::Lap);

    // Reset idle timer
    model->resetIdleTimer();

    view.setUnitsImperial(model->isUnitsImperial());

    const Model::LapInfo& lap = model->getLap();

    view.setLapNum(lap.num);
    view.setDistance(lap.distance);
    view.setTimer(lap.time);
    view.setPace(lap.pace);
    view.setDelta(lap.delta);
}

void TrackLapPresenter::deactivate()
{
    model->setLapFromPauseMenu(false);
}

void TrackLapPresenter::dismiss()
{
    if (model->isLapFromPauseMenu()) {
        model->application().gotoTrackActionScreenNoTransition();
    } else {
        model->application().gotoTrackScreenNoTransition();
    }
}
