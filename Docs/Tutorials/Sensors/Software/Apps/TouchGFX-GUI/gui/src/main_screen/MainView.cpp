#include <gui/main_screen/MainView.hpp>

#define LOG_MODULE_PRX      "MainView"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

MainView::MainView()
    : verbosity(BASIC),
      hr(0), hrtl(0),
      gpsLat(0), gpsLon(0), gpsAlt(0),
        elevation(0),
        altPressure(0), altAltitude(0),
        magX(0), magY(0), magZ(0),
       accX(0), accY(0), accZ(0),
      steps(0), floors(0),
       heading(0),
      rtcTime(0),
      serviceCpu(0), guiCpu(0), txMsgPerSec(0), rxMsgPerSec(0), txBytesPerSec(0), rxBytesPerSec(0),
      batteryLevel(0)
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(ButtonsSet::NONE);
    buttons.setL2(ButtonsSet::NONE);
    buttons.setR1(ButtonsSet::NONE);
    buttons.setR2(ButtonsSet::AMBER);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleTickEvent()
{
    refreshDisplay();
    refreshStats();
    refreshBattery();
}

void MainView::updateHR(float hr, float tl)
{
    this->hr = hr;
    this->hrtl = tl;
    LOG_DEBUG("HR: %f BPM\n", hr);
}

void MainView::updateGPS(float lat, float lon, float alt)
{
    this->gpsLat = lat;
    this->gpsLon = lon;
    this->gpsAlt = alt;
}

void MainView::updateElevation(float elevation)
{
    this->elevation = elevation;
}

void MainView::updateAccelerometer(float x, float y, float z)
{
    this->accX = x;
    this->accY = y;
    this->accZ = z;
}

void MainView::updateStepCounter(uint32_t steps)
{
    this->steps = steps;
}

void MainView::updateFloorCounter(uint32_t floors)
{
    this->floors = floors;
}

void MainView::updateCompass(float heading)
{
    this->heading = heading;
}

void MainView::updateRTC(uint32_t time)
{
    this->rtcTime = time;
}

void MainView::updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec)
{
    this->serviceCpu = serviceCpu;
    this->guiCpu = guiCpu;
    this->txMsgPerSec = txMsgPerSec;
    this->rxMsgPerSec = rxMsgPerSec;
    this->txBytesPerSec = txBytesPerSec;
    this->rxBytesPerSec = rxBytesPerSec;
}

void MainView::updateBattery(float level)
{
    this->batteryLevel = level;
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        verbosity = static_cast<VerbosityLevel>((verbosity + 1) % 3);
        LOG_DEBUG("Verbosity changed to %d\n", (int)verbosity);
    }

    if (key == Gui::Config::Button::L2) {
        verbosity = static_cast<VerbosityLevel>((verbosity - 1 + 3) % 3);
        LOG_DEBUG("Verbosity changed to %d\n", (int)verbosity);
    }

    if (key == Gui::Config::Button::R1) {
   
    }

    if (key == Gui::Config::Button::R2) {
        presenter->exit();
    }
}

void MainView::refreshDisplay()
{
    // Format sensor data based on verbosity
    int pos = 0;

    // Time
    pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Time: %lu\n", rtcTime));

    if (verbosity >= BASIC) {
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "HR: %f BPM\n", hr));
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Steps: %lu\n", steps));
    }

    if (verbosity >= DETAILED) {
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "GPS: %f, %f, %f\n", gpsLat, gpsLon, gpsAlt));
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Alt: %f Pa, %f m\n", altPressure, altAltitude));
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Acc: %f, %f, %f\n", accX, accY, accZ));
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Floors: %lu\n", floors));
    }

    if (verbosity >= FULL) {
        pos += Unicode::strlen(Unicode::snprintf(text_bodyBuffer + pos, TEXT_BODY_SIZE - pos, "Mag: %f, %f, %f\n", magX, magY, magZ));
    }

    text_body.invalidate();
}

void MainView::refreshStats()
{
    Unicode::snprintf(text_statsBuffer, TEXT_STATS_SIZE, "CPU S: %f%% G: %f%%\nMsg Tx: %f Rx: %f\nBytes Tx: %f Rx: %f",
                      serviceCpu, guiCpu, txMsgPerSec, rxMsgPerSec, txBytesPerSec, rxBytesPerSec);
    text_stats.invalidate();
}

void MainView::refreshBattery()
{
    Unicode::snprintf(text_headerBuffer, TEXT_HEADER_SIZE, "Battery: %f%%", batteryLevel);
    text_header.invalidate();
}