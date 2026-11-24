#include <gui/trackrunalert_screen/TrackRunAlertView.hpp>
#include <gui/trackrunalert_screen/TrackRunAlertPresenter.hpp>

TrackRunAlertPresenter::TrackRunAlertPresenter(TrackRunAlertView& v)
    : view(v)
{
}

void TrackRunAlertPresenter::activate()
{
    model->resetIdleTimer();

    const Model::RunAlertInfo& a = model->getRunAlert();
    view.setAlert(a.kind, a.value, model->isUnitsImperial());
}

void TrackRunAlertPresenter::deactivate()
{
}
