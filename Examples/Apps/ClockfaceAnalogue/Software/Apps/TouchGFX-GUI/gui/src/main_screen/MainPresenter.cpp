#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{

}

void MainPresenter::deactivate()
{

}

void MainPresenter::onTime(const WallTime &time)
{
    view.setTime(time);
}

void MainPresenter::onBatteryLevel(uint8_t level)
{
    view.setBatteryLevel(level);
}

void MainPresenter::onAlertsMuted(bool muted)
{
    view.setAlertsMuted(muted);
}
