#include <gui/running_screen/RunningView.hpp>
#include <gui/running_screen/RunningPresenter.hpp>

RunningPresenter::RunningPresenter(RunningView& v)
    : view(v)
{
}

void RunningPresenter::activate()
{
    model->resetIdleTimer();
    view.onStateChanged();   // initial sync to the current countdown state
}

void RunningPresenter::deactivate()
{
}

void RunningPresenter::onStateChanged()
{
    view.onStateChanged();
}
