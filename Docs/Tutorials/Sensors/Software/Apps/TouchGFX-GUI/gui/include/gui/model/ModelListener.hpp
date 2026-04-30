#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void onIdleTimeout() {}

    virtual void updateHR(float hr, float tl) {}

    virtual void updateGPS(float lat, float lon, float alt) {}
    virtual void updateElevation(float elevation) {}
    virtual void updateAccelerometer(float x, float y, float z) {}
    virtual void updateStepCounter(uint32_t steps) {}
    virtual void updateFloorCounter(uint32_t floors) {}
    virtual void updateCompass(float heading) {}
    virtual void updateRTC(uint32_t time) {}
    virtual void updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec) {}
    virtual void updateBattery(float level) {}
    virtual void updatePressure(float pressure) {}

protected:
    Model* model;

    
};

#endif // MODELLISTENER_HPP
