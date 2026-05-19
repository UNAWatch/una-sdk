#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{
    view.setPositionId(model->menu().get());
    model->menu().resetChildren();
    model->resetIdleTimer();

    view.setGpsFix(model->hasGpsFix());
}

void MainPresenter::deactivate()
{
    model->menu().set(view.getPositionId());
}

void MainPresenter::onIdleTimeout()
{
    if (view.getPositionId() != App::MenuNav::Root::ID_START) {
        model->exitApp();
    }
}

void MainPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MainPresenter::startTrack()
{
    model->trackStart();
}

void MainPresenter::exitApp()
{
    model->exitApp();
}
