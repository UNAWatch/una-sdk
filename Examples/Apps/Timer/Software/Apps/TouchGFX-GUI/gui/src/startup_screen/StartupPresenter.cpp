#include <gui/startup_screen/StartupView.hpp>
#include <gui/startup_screen/StartupPresenter.hpp>

StartupPresenter::StartupPresenter(StartupView& v)
    : view(v)
{

}

void StartupPresenter::activate()
{
    model->resetIdleTimer();
}

void StartupPresenter::deactivate()
{

}
