/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen: one per sensor
 *          message the service sends, plus the kernel's frame tick.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include <cstdint>

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// One call per kernel frame (10 Hz on the watch): the TouchGFX
    /// handleTickEvent(), delivered through the model.
    virtual void onFrame() {}

    virtual void updateHR(float hr, float tl) { (void)hr; (void)tl; }
    virtual void updateGPS(float lat, float lon, float alt) { (void)lat; (void)lon; (void)alt; }
    virtual void updateElevation(float elevation) { (void)elevation; }
    virtual void updateAccelerometer(float x, float y, float z) { (void)x; (void)y; (void)z; }
    virtual void updateStepCounter(uint32_t steps) { (void)steps; }
    virtual void updateFloorCounter(uint32_t floors) { (void)floors; }
    virtual void updateCompass(float heading) { (void)heading; }
    virtual void updateRTC(uint32_t time) { (void)time; }
    virtual void updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec,
                             float txBytesPerSec, float rxBytesPerSec)
    {
        (void)serviceCpu; (void)guiCpu; (void)txMsgPerSec; (void)rxMsgPerSec; (void)txBytesPerSec; (void)rxBytesPerSec;
    }
    virtual void updateBattery(float level) { (void)level; }
    virtual void updatePressure(float pressure) { (void)pressure; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
