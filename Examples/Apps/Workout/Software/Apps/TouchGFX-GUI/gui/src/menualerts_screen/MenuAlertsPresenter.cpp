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

    view.setAccessoryStatus(model->getAccessoryState(), "");
    view.setTime(model->getSettings().alertTimeId);
}

void MenuAlertsPresenter::deactivate()
{
    model->menu().settings.alerts.set(view.getPositionId());
}

void MenuAlertsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}
