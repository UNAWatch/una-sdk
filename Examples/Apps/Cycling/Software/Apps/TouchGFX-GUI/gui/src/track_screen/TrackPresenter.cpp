#include <gui/track_screen/TrackView.hpp>
#include <gui/track_screen/TrackPresenter.hpp>

TrackPresenter::TrackPresenter(TrackView& v)
    : view(v)
{

}

void TrackPresenter::activate()
{
    // Reset nested action menu position
    model->menu().track.action.reset();

    view.setPositionId(model->menu().track.get());

    view.setConfig(model->isUnitsImperial(), model->getHrThresholds(), model->getHrThresholdsCount());
    view.setTimeFormat(model->is12HourFormat());

    onTrackData(model->getTrackData());

    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
    model->getTime(hour, minute, sec);
    view.setTime(hour, minute);

    view.setBatteryLevel(model->getBatteryLevel());
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState());

    // The track can already be paused when this screen is entered -- auto-pause
    // may have fired while another screen was up -- so adopt the current state
    // and the pause duration the Model has been accumulating meanwhile, rather
    // than assuming active and restarting the count.
    view.setPaused(model->isTrackPaused());
    view.setPausedTime(model->getPausedSeconds());
}

void TrackPresenter::deactivate()
{
    model->menu().track.set(view.getPositionId());
}

void TrackPresenter::onTrackData(const Track::Data& data)
{
    view.setTrackData(data);
}

void TrackPresenter::onBatteryLevel(uint8_t lvl)
{
    view.setBatteryLevel(lvl);
}

void TrackPresenter::onTime(uint8_t hour, uint8_t minute, uint8_t sec)
{
    view.setTime(hour, minute);

    // onTime() is the only 1 Hz tick the GUI still receives while paused: the
    // activity timer is frozen, so TRACK_DATA_UPDATE carries a constant value.
    if (model->isTrackPaused()) {
        view.setPausedTime(model->getPausedSeconds());
    }
}

void TrackPresenter::onTrackState(const Track::State& state)
{
    view.setPaused(state == Track::State::PAUSED);
    view.setPausedTime(model->getPausedSeconds());
}

void TrackPresenter::onLapChanged(uint8_t lapEnd)
{
    model->application().gotoTrackLapScreenNoTransition();
}

void TrackPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void TrackPresenter::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    view.setAccessoryStatus(state);
}

void TrackPresenter::saveLap()
{
    model->saveLap();
}
