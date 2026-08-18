#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{
    // The service sends an update as soon as the GUI starts, but a screen that
    // is switched in later would otherwise wait for the next GPS fix.
    view.showNav(model->nav());
}

void MainPresenter::deactivate()
{

}

void MainPresenter::onNavUpdate(const CustomMessage::NavState& nav)
{
    view.showNav(nav);
}

void MainPresenter::onTargetSaved(bool saved, float latitude, float longitude)
{
    view.showTargetSaved(saved, latitude, longitude);
}
