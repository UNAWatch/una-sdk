#include <gui/menualerts_screen/MenuAlertsView.hpp>
#include <gui/menualerts_screen/MenuAlertsPresenter.hpp>

MenuAlertsPresenter::MenuAlertsPresenter(MenuAlertsView& v)
    : view(v)
{

}

void MenuAlertsPresenter::activate()
{
    view.setPositionId(model->menu().settings.alerts.get());
    model->resetIdleTimer();

    view.setUnitsImperial(model->isUnitsImperial());
    view.setDistance(model->getSettings().alertDistanceId);
    view.setTime(model->getSettings().alertTimeId);
}

void MenuAlertsPresenter::deactivate()
{
    model->menu().settings.alerts.set(view.getPositionId());
}
