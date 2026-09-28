/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Sensors and its link to the service.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    // The port owns the kernel's callback slots and forwards to these.
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator!\n");
    LOG_INFO("Keys: 1 = L1 (more detail), 2 = L2 (less detail), 4 = R2 (exit), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onFrame()
{
    // The kernel's tick, once per frame; the screen redraws from it.
    if (modelListener) {
        modelListener->onFrame();
    }
}

void Model::onResume()
{
    LOG_INFO("called\n");
}

void Model::onSuspend()
{
    LOG_INFO("called\n");
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

// Events from the service. Each message type maps to one listener call; the
// service sends them as the sensors deliver, so a screen should store the
// values and draw on its own schedule rather than redraw per message.
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    if (!modelListener) {
        return true;
    }
    switch (msg->getType()) {
        case CustomMessage::HR_VALUES: {
            auto* m = static_cast<CustomMessage::HRValues*>(msg);
            LOG_DEBUG("hr %.1f, tl %.1f\n", m->heartRate, m->trustLevel);
            modelListener->updateHR(m->heartRate, m->trustLevel);
        } break;

        case CustomMessage::LOCATION_VALUES: {
            auto* m = static_cast<CustomMessage::LocationValues*>(msg);
            modelListener->updateGPS(static_cast<float>(m->latitude), static_cast<float>(m->longitude),
                                     static_cast<float>(m->altitude));
        } break;

        case CustomMessage::ELEVATION_VALUES: {
            auto* m = static_cast<CustomMessage::ElevationValues*>(msg);
            modelListener->updateElevation(m->elevation);
        } break;

        case CustomMessage::ACCELEROMETER_VALUES: {
            auto* m = static_cast<CustomMessage::AccelerometerValues*>(msg);
            modelListener->updateAccelerometer(m->x, m->y, m->z);
        } break;

        case CustomMessage::STEP_COUNTER_VALUES: {
            auto* m = static_cast<CustomMessage::StepCounterValues*>(msg);
            modelListener->updateStepCounter(m->steps);
        } break;

        case CustomMessage::FLOORS_VALUES: {
            auto* m = static_cast<CustomMessage::FloorsValues*>(msg);
            modelListener->updateFloorCounter(m->floors);
        } break;

        case CustomMessage::COMPASS_VALUES: {
            auto* m = static_cast<CustomMessage::CompassValues*>(msg);
            modelListener->updateCompass(m->heading);
        } break;

        case CustomMessage::STATS_VALUES: {
            auto* m = static_cast<CustomMessage::StatsValues*>(msg);
            modelListener->updateStats(m->serviceCpuPct, m->guiCpuPct, m->txMsgRate, m->rxMsgRate,
                                       m->txByteRate, m->rxByteRate);
        } break;

        case CustomMessage::RTC_VALUES: {
            auto* m = static_cast<CustomMessage::RtcValues*>(msg);
            modelListener->updateRTC(m->time);
        } break;

        case CustomMessage::BATTERY_VALUES: {
            auto* m = static_cast<CustomMessage::BatteryValues*>(msg);
            modelListener->updateBattery(m->level);
        } break;

        case CustomMessage::PRESSURE_VALUES: {
            auto* m = static_cast<CustomMessage::PressureValues*>(msg);
            modelListener->updatePressure(m->pressure);
        } break;

        default:
            break;
    }
    return true;
}
