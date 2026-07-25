#include <gui/alert_screen/AlertView.hpp>
#include <gui/alert_screen/AlertPresenter.hpp>

AlertPresenter::AlertPresenter(AlertView& v)
    : view(v)
{
}

void AlertPresenter::activate()
{
    model->resetIdleTimer();
}

void AlertPresenter::deactivate()
{
}
