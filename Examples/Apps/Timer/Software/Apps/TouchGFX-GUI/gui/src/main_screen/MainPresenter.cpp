#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{
}

void MainPresenter::activate()
{
    model->resetIdleTimer();
    view.setLists(model->getPresets(), model->getRecents());
}

void MainPresenter::deactivate()
{
}

void MainPresenter::onRecentsChanged(const std::vector<Timer>& list)
{
    view.setLists(model->getPresets(), list);
}

void MainPresenter::selectTimer(const Timer& timer, int16_t index)
{
    model->setEditTimer(timer);
    model->requestRestoreSelection(index);   // Main re-selects it when we come back
}

void MainPresenter::editNew()
{
    model->setEditTimer(Timer{ 60, Timer::EFFECT_BEEP_AND_VIBRO });
}

void MainPresenter::exitApp()
{
    model->exitApp();
}
