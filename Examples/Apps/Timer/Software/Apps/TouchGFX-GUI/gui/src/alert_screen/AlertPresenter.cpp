#include <gui/alert_screen/AlertView.hpp>
#include <gui/alert_screen/AlertPresenter.hpp>

AlertPresenter::AlertPresenter(AlertView& v)
    : view(v)
{
}

void AlertPresenter::activate()
{
    model->resetIdleTimer();
    view.set(model->getEditTimer().effect);
}

void AlertPresenter::deactivate()
{
}

void AlertPresenter::setEffect(Timer::Effect effect)
{
    Timer t = model->getEditTimer();
    t.effect = effect;
    model->setEditTimer(t);
}
