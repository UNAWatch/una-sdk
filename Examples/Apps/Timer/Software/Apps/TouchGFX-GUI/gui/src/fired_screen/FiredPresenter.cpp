#include <gui/fired_screen/FiredView.hpp>
#include <gui/fired_screen/FiredPresenter.hpp>

FiredPresenter::FiredPresenter(FiredView& v)
    : view(v)
{
}

void FiredPresenter::activate()
{
    model->resetIdleTimer();
}

void FiredPresenter::deactivate()
{
}
