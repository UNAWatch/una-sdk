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

void MainPresenter::selectTimer(const Timer& timer)
{
    // Record the timer to act on; the restore request is raised on the way back
    // out of Menu (see MenuPresenter/DeletedPresenter), so every path into Menu
    // -- not just this one -- re-selects it on Main.
    model->setEditTimer(timer);
}

void MainPresenter::editNew()
{
    model->setEditTimer(Timer{ 60, Timer::EFFECT_BEEP_AND_VIBRO });
}

void MainPresenter::exitApp()
{
    model->exitApp();
}
