#include <gui/trackcalibrateintro_screen/TrackCalibrateIntroView.hpp>
#include <gui/trackcalibrateintro_screen/TrackCalibrateIntroPresenter.hpp>

TrackCalibrateIntroPresenter::TrackCalibrateIntroPresenter(TrackCalibrateIntroView& v)
    : view(v)
{
}

void TrackCalibrateIntroPresenter::activate()
{
    model->resetIdleTimer();
}

void TrackCalibrateIntroPresenter::deactivate()
{
}
