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

void MainPresenter::updateHR(float hr, float tl)
{
    view.updateHR(hr, tl);
}

void MainPresenter::updateGPS(float lat, float lon, float alt)
{
    view.updateGPS(lat, lon, alt);
}

void MainPresenter::updateElevation(float elevation)
{
    view.updateElevation(elevation);
}

void MainPresenter::updateAccelerometer(float x, float y, float z)
{
    view.updateAccelerometer(x, y, z);
}

void MainPresenter::updateStepCounter(uint32_t steps)
{
    view.updateStepCounter(steps);
}

void MainPresenter::updateFloorCounter(uint32_t floors)
{
    view.updateFloorCounter(floors);
}

void MainPresenter::updateCompass(float heading)
{
    view.updateCompass(heading);
}

void MainPresenter::updateRTC(uint32_t time)
{
    view.updateRTC(time);
}

void MainPresenter::updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec)
{
    view.updateStats(serviceCpu, guiCpu, txMsgPerSec, rxMsgPerSec, txBytesPerSec, rxBytesPerSec);
}

void MainPresenter::updateBattery(float level)
{
    view.updateBattery(level);
}