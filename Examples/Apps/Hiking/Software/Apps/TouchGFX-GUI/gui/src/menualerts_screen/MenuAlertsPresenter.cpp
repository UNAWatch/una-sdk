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
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuAlertsPresenter::deactivate()
{
    model->menu().settings.alerts.set(view.getPositionId());
}

void MenuAlertsPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuAlertsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}
