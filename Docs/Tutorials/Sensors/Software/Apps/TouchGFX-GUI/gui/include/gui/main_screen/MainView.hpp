#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void updateHR(float hr, float tl);

    void updateGPS(float lat, float lon, float alt);
    void updateElevation(float elevation);
    void updateAccelerometer(float x, float y, float z);
    void updateStepCounter(uint32_t steps);
    void updateFloorCounter(uint32_t floors);
    void updateCompass(float heading);
    void updateRTC(uint32_t time);
    void updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec);
    void updateBattery(float level);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    enum VerbosityLevel { BASIC, DETAILED, FULL };
    VerbosityLevel verbosity;

    // Sensor data
    float hr, hrtl;
    float gpsLat, gpsLon, gpsAlt;
    float elevation;
    float altPressure, altAltitude;
    float magX, magY, magZ;
    float accX, accY, accZ;
    uint32_t steps, floors;
    float heading;
    uint32_t rtcTime;
    float serviceCpu, guiCpu, txMsgPerSec, rxMsgPerSec, txBytesPerSec, rxBytesPerSec;
    float batteryLevel;

    void refreshDisplay();
    void refreshStats();
    void refreshBattery();
};

#endif // MAINVIEW_HPP
