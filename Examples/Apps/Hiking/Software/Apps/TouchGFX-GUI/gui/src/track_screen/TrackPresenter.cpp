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

    onTrackData(model->getTrackData());

    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
    model->getTime(hour, minute, sec);
    view.setTime(hour, minute);

    view.setBatteryLevel(model->getBatteryLevel());
    view.setGpsFix(model->hasGpsFix());
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
}

void TrackPresenter::onLapChanged(uint8_t lapEnd)
{
    model->application().gotoTrackLapScreenNoTransition();
}

void TrackPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void TrackPresenter::saveLap()
{
    model->saveLap();
}
