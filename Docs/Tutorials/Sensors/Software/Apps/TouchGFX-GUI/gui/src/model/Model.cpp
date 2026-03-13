#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#if defined(SIMULATOR)
    #include "touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp"
    #include "Windows.h"
    #include <chrono>
    #include <ctime>
#endif

Model::Model()
    : modelListener(0)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator! \n");

    std::string fileStoreDir = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Path to files created by app:\n"
        "       [%s]\n", fileStoreDir.c_str());

    LOG_INFO("\n"
        "       Keys:                       \n"
        "       ----------------------------\n"
        "       1   L1,                     \n"
        "       2   L2,                     \n"
        "       3   R1,                     \n"
        "       4   R2,                     \n"
        "       z   L1+R2                   \n"
    );
#endif
}

FrontendApplication& Model::application()
{
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

void Model::tick()
{
    //LOG_DEBUG("called\n");

    if (mInvalidate) {
        mInvalidate = false;
        application().invalidate();
    }
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    // Cleanup recourses

    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);

    mKernel.sys.exit(); // No return for real app

    // !!! For TouchGFX Simulator !!!
    // This function only sets a flag.
    // The current TouchGFX loop will be completed, meaning that depending
    // on where this function was called, Model::tick(), Model::handleKeyEvent(),
    // as well as handleTickEvent() and handleKeyEvent() for the
    // current screen will be called.
}

// IUserApp implementation
void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    LOG_INFO("called\n");

    // Redraw screen
    mInvalidate = true;
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

void Model::onSuspend()
{
    LOG_INFO("called\n");
}

// Events from Service

bool Model::customMessageHandler(SDK::MessageBase *msg)
{
    switch (msg->getType()) {
        case CustomMessage::HR_VALUES:  {
            auto* m = static_cast<CustomMessage::HRValues*>(msg);
            LOG_DEBUG("hr %.1f, tl %.1f\n", m->heartRate, m->trustLevel);
            modelListener->updateHR(m->heartRate, m->trustLevel);
        } break;

        case CustomMessage::LOCATION_VALUES:  {
            auto* m = static_cast<CustomMessage::LocationValues*>(msg);
            LOG_DEBUG("GPS: %.6f, %.6f, %.1f\n", m->latitude, m->longitude, m->altitude);
            modelListener->updateGPS(m->latitude, m->longitude, m->altitude);
        } break;

        case CustomMessage::ELEVATION_VALUES:  {
            auto* m = static_cast<CustomMessage::ElevationValues*>(msg);
            LOG_DEBUG("Elevation: %.1f m\n", m->elevation);
            modelListener->updateElevation(m->elevation);
        } break;

        case CustomMessage::ACCELEROMETER_VALUES:  {
            auto* m = static_cast<CustomMessage::AccelerometerValues*>(msg);
            LOG_DEBUG("Acc: %.2f, %.2f, %.2f\n", m->x, m->y, m->z);
            modelListener->updateAccelerometer(m->x, m->y, m->z);
        } break;

        case CustomMessage::STEP_COUNTER_VALUES:  {
            auto* m = static_cast<CustomMessage::StepCounterValues*>(msg);
            LOG_DEBUG("Steps: %u\n", m->steps);
            modelListener->updateStepCounter(m->steps);
        } break;

        case CustomMessage::FLOORS_VALUES:  {
            auto* m = static_cast<CustomMessage::FloorsValues*>(msg);
            LOG_DEBUG("Floors: %u\n", m->floors);
            modelListener->updateFloorCounter(m->floors);
        } break;

        case CustomMessage::COMPASS_VALUES:  {
            auto* m = static_cast<CustomMessage::CompassValues*>(msg);
            LOG_DEBUG("Compass: %.1f\n", m->heading);
            modelListener->updateCompass(m->heading);
        } break;

        case CustomMessage::STATS_VALUES:  {
            auto* m = static_cast<CustomMessage::StatsValues*>(msg);
            LOG_DEBUG("Stats: S%.1f%% G%.1f%% TX:%.0f msg/s (%.0f B/s) RX:%.0f msg/s (%.0f B/s)\n",
                      m->serviceCpuPct, m->guiCpuPct,
                      m->txMsgRate, m->txByteRate,
                      m->rxMsgRate, m->rxByteRate);
            modelListener->updateStats(m->serviceCpuPct, m->guiCpuPct,
                                       m->txMsgRate, m->rxMsgRate,
                                       m->txByteRate, m->rxByteRate);
        } break;

        case CustomMessage::RTC_VALUES:  {
            auto* m = static_cast<CustomMessage::RtcValues*>(msg);
            LOG_DEBUG("RTC: %u\n", m->time);
            modelListener->updateRTC(m->time);
        } break;

        case CustomMessage::BATTERY_VALUES:  {
            auto* m = static_cast<CustomMessage::BatteryValues*>(msg);
            LOG_DEBUG("Battery: %.1f%%\n", m->level);
            modelListener->updateBattery(m->level);
        } break;

        case CustomMessage::PRESSURE_VALUES:  {
            auto* m = static_cast<CustomMessage::PressureValues*>(msg);
            LOG_DEBUG("Pressure: %.1f hPa\n", m->pressure);
            modelListener->updatePressure(m->pressure);
        } break;

        default:
            break;
    }

    return true;
}
