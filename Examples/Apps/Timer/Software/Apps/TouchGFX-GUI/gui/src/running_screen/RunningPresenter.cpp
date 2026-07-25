#include <gui/running_screen/RunningView.hpp>
#include <gui/running_screen/RunningPresenter.hpp>

RunningPresenter::RunningPresenter(RunningView& v)
    : view(v)
{
}

void RunningPresenter::activate()
{
    model->resetIdleTimer();
}

void RunningPresenter::deactivate()
{
}
