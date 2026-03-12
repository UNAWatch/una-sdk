#include <gui/main_screen/MainView.hpp>

#define LOG_MODULE_PRX      "MainView"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

MainView::MainView()
    : verbosity(FULL),
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
      batteryLevel(0),
      mFrameCounter(0)
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
    LOG_DEBUG("Display Refreshed!\nFrenme No: %u\n------------- END ------------\n", mFrameCounter++);
}

void MainView::updateHR(float hr, float tl)
{
    this->hr = hr;
    this->hrtl = tl;
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
        verbosity = static_cast<VerbosityLevel>((verbosity + 1) % VERB_LEVEL_MAX);
        LOG_DEBUG("Verbosity changed to %d\n", (int)verbosity);
    }

    if (key == Gui::Config::Button::L2) {
        verbosity = static_cast<VerbosityLevel>((verbosity - 1 + VERB_LEVEL_MAX) % VERB_LEVEL_MAX);
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
    char buffer[256];
    int len = 0;

    // Time
    if (verbosity <= FULL) {
        // Group display
        if (verbosity >= BASIC) {
            len += snprintf(buffer + len, sizeof(buffer) - len, "HR: %.0f BPM\n", hr);
            len += snprintf(buffer + len, sizeof(buffer) - len, "Steps: %lu\n", steps);
        }
        if (verbosity >= DETAILED) {
            len += snprintf(buffer + len, sizeof(buffer) - len, "GPS: %.2f, %.2f, %.0f\n", gpsLat, gpsLon, gpsAlt);
            len += snprintf(buffer + len, sizeof(buffer) - len, "Elev: %.1f m\n", elevation);
            len += snprintf(buffer + len, sizeof(buffer) - len, "Acc: %.2f, %.2f, %.2f\n", accX, accY, accZ);
            len += snprintf(buffer + len, sizeof(buffer) - len, "Floors: %lu\n", floors);
        }
        if (verbosity >= FULL) {
            len += snprintf(buffer + len, sizeof(buffer) - len, "Compass: %.0f°\n", heading);
        }
    } else if (verbosity < VERB_LEVEL_MAX) {
        // Per-sensor detailed display
        switch (verbosity) {
            case HR:
                len += snprintf(buffer + len, sizeof(buffer) - len, "HR: %.0f BPM\nTL: %.0f\n", hr, hrtl);
                break;
            case GPS:
                len += snprintf(buffer + len, sizeof(buffer) - len, "GPS: %.6f, %.6f\nAlt: %.1f m\n", gpsLat, gpsLon, gpsAlt);
                break;
            case ALT:
                len += snprintf(buffer + len, sizeof(buffer) - len, "Elevation: %.1f m\n", elevation);
                break;
            case ACC:
                len += snprintf(buffer + len, sizeof(buffer) - len, "Accelerometer:\nX: %.2f G\nY: %.2f G\nZ: %.2f G\n", accX, accY, accZ);
                break;
            case STEP:
                len += snprintf(buffer + len, sizeof(buffer) - len, "Steps: %lu\n", steps);
                break;
            case FLOOR:
                len += snprintf(buffer + len, sizeof(buffer) - len, "Floors: %lu\n", floors);
                break;
            case MAG:
                len += snprintf(buffer + len, sizeof(buffer) - len, "Compass: %.0f\n", heading);
                break;
        }
    }

    if (verbosity > FULL && verbosity < VERB_LEVEL_MAX) {
        // Smaller text size 50 for per-sensor
        text_body.setTypedText(TypedText(T_TMP_REGULAR_18));
    } else {
        // Default size
        text_body.setTypedText(TypedText(T_TMP_REGULAR_9)); // assume exists
    }
    LOG_DEBUG("Current verbosity: %d\\n", (int)verbosity);
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
    auto len = snprintf(buffer, sizeof(buffer), "Battery: %.1f%%\n", batteryLevel);
    len += snprintf(buffer + len, sizeof(buffer) - len, "# %lu\nTime: %lu\n", mFrameCounter, rtcTime);
    Unicode::strncpy(text_headerBuffer, buffer, TEXT_HEADER_SIZE);
    text_header.invalidate();
}