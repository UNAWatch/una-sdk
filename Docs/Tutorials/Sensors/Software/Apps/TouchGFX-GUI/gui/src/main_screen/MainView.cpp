#include <gui/main_screen/MainView.hpp>

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

void MainView::updateHR(float hr, float tl)
{
    this->hr = hr;
    this->hrtl = tl;
    refreshDisplay();
}

void MainView::updateGPS(float lat, float lon, float alt)
{
    this->gpsLat = lat;
    this->gpsLon = lon;
    this->gpsAlt = alt;
    refreshDisplay();
}

void MainView::updateElevation(float elevation)
{
    this->elevation = elevation;
    refreshDisplay();
}

void MainView::updateAccelerometer(float x, float y, float z)
{
    this->accX = x;
    this->accY = y;
    this->accZ = z;
    refreshDisplay();
}

void MainView::updateStepCounter(uint32_t steps)
{
    this->steps = steps;
    refreshDisplay();
}

void MainView::updateFloorCounter(uint32_t floors)
{
    this->floors = floors;
    refreshDisplay();
}

void MainView::updateCompass(float heading)
{
    this->heading = heading;
    refreshDisplay();
}

void MainView::updateRTC(uint32_t time)
{
    this->rtcTime = time;
    refreshDisplay();
}

void MainView::updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec)
{
    this->serviceCpu = serviceCpu;
    this->guiCpu = guiCpu;
    this->txMsgPerSec = txMsgPerSec;
    this->rxMsgPerSec = rxMsgPerSec;
    this->txBytesPerSec = txBytesPerSec;
    this->rxBytesPerSec = rxBytesPerSec;
    refreshStats();
}

void MainView::updateBattery(float level)
{
    this->batteryLevel = level;
    refreshBattery();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        verbosity = static_cast<VerbosityLevel>((verbosity + 1) % 3);
        refreshDisplay();
    }

    if (key == Gui::Config::Button::L2) {
        verbosity = static_cast<VerbosityLevel>((verbosity - 1 + 3) % 3);
        refreshDisplay();
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
    char buffer[256];
    int len = 0;

    // Time
    len += snprintf(buffer + len, sizeof(buffer) - len, "Time: %lu\n", rtcTime);

    if (verbosity >= BASIC) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "HR: %.0f BPM\n", hr);
        len += snprintf(buffer + len, sizeof(buffer) - len, "Steps: %lu\n", steps);
    }

    if (verbosity >= DETAILED) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "GPS: %.2f, %.2f, %.0f\n", gpsLat, gpsLon, gpsAlt);
        len += snprintf(buffer + len, sizeof(buffer) - len, "Alt: %.1f Pa, %.1f m\n", altPressure, altAltitude);
        len += snprintf(buffer + len, sizeof(buffer) - len, "Acc: %.2f, %.2f, %.2f\n", accX, accY, accZ);
        len += snprintf(buffer + len, sizeof(buffer) - len, "Floors: %lu\n", floors);
    }

    if (verbosity >= FULL) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "Mag: %.2f, %.2f, %.2f\n", magX, magY, magZ);
    }

    Unicode::strncpy(text_bodyBuffer, buffer, TEXT_BODY_SIZE);
    text_body.invalidate();
}

void MainView::refreshStats()
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "CPU S: %.1f%% G: %.1f%%\nMsg Tx: %.0f Rx: %.0f\nBytes Tx: %.0f Rx: %.0f",
             serviceCpu, guiCpu, txMsgPerSec, rxMsgPerSec, txBytesPerSec, rxBytesPerSec);
    Unicode::strncpy(text_statsBuffer, buffer, TEXT_STATS_SIZE);
    text_stats.invalidate();
}

void MainView::refreshBattery()
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Battery: %.1f%%", batteryLevel);
    Unicode::strncpy(text_headerBuffer, buffer, TEXT_HEADER_SIZE);
    text_header.invalidate();
}