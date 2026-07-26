#include <gui/menu_screen/MenuView.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>

MenuPresenter::MenuPresenter(MenuView& v)
    : view(v)
{
}

void MenuPresenter::activate()
{
    model->resetIdleTimer();
    view.setValue(model->getEditTimer().durationSec);
}

void MenuPresenter::deactivate()
{
}

void MenuPresenter::startTimer()
{
    model->startTimer(model->getEditTimer());
}
