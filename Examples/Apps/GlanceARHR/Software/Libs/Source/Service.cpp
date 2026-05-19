#include "Service.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateMetrics.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

Service::Service(SDK::Kernel &kernel)
    : mKernel(kernel)
    , mGlanceUI()
    , mGlanceTitle()
    , mGlanceValueAHR()
    , mGlanceValueRHR()
    , mSensorHRMetrics(SDK::Sensor::Type::HEART_RATE_METRICS_DAILY)
    , mAvgHrValue(0)
    , mRestHrValue(0)
    , mDataReceived(false)
{
}

Service::~Service()
{
    disconnect();
}

void Service::run()
{
    LOG_INFO("Started\n");

    while (true) {
        SDK::MessageBase *msg;

        // Wait for command (blocks until available)
        if(!mKernel.comm.getMessage(msg)) {
            continue;
        }

        switch (msg->getType()) {

            case SDK::MessageType::EVENT_GLANCE_START:
                LOG_INFO("GLANCE is now running\n");
                if (configGui()) {
                    createGuiControls();
                    connect();
                } else {
                    mKernel.comm.releaseMessage(msg);
                    mKernel.sys.exit(0); // no return
                }
                break;

            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
            case SDK::MessageType::EVENT_GLANCE_STOP:
                LOG_INFO("GLANCE has stopped\n");
                disconnect();
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;
                break;

            case SDK::MessageType::EVENT_GLANCE_TICK:
                onGlanceTick();
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                handleSensorsData(event->handle, batch);
                } break;

            default:
                break;
        }

        // Release message after processing
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::connect()
{
    if (!mSensorHRMetrics.isConnected()) {
        LOG_DEBUG("Connect to sensors...\n");
        mSensorHRMetrics.connect();
    }
}

void Service::disconnect()
{
    if (mSensorHRMetrics.isConnected()) {
        LOG_DEBUG("Disconnect from sensors...\n");
        mSensorHRMetrics.disconnect();
    }
}

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorHRMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateMetrics p(data[0]);

        if (!p.isDataValid()) {
            return;
        }

        float newAHRValue = p.getAhr();
        float newRHRValue = p.getRhr();

        LOG_DEBUG("AHR: %.1f, RHR: %.1f\n", newAHRValue, newRHRValue);

        if ((static_cast<uint32_t>(newAHRValue) != mAvgHrValue) ||
            (static_cast<uint32_t>(newRHRValue) != mRestHrValue) ||
            !mDataReceived)
        {
            mDataReceived = true;
            mAvgHrValue = static_cast<uint32_t>(newAHRValue);
            mRestHrValue = static_cast<uint32_t>(newRHRValue);
            glanceUpdate();
        }
    }
}

void Service::onGlanceTick()
{
    if (mGlanceUI.isInvalid()) {
        if (auto upd = SDK::make_msg<SDK::Message::RequestGlanceUpdate>(mKernel)) {
            upd->name           = APP_NAME;
            upd->controls       = mGlanceUI.data();
            upd->controlsNumber = static_cast<uint32_t>(mGlanceUI.size());
            upd.send(100);
        }

        mGlanceUI.setValid();
   }
}

bool Service::configGui()
{
    bool status = false;
    // Get Glance configuration
    if (auto gc = SDK::make_msg<SDK::Message::RequestGlanceConfig>(mKernel)) {
        if (gc.send(100) && gc.ok()) {
            if (gc->maxControls >= 3) {
                mGlanceUI.setWidth(gc->width);
                mGlanceUI.setHeight(gc->height);
                status = true;
            }
        }
    }

    return status;
}

void Service::glanceUpdate()
{
    mGlanceValueAHR.print("%u", mAvgHrValue);
    mGlanceValueRHR.print("%u", mRestHrValue);
}

void Service::createGuiControls()
{
    mGlanceTitle = mGlanceUI.createText();

    mGlanceTitle
        .pos({ kTitleX, kTitleY }, { kTitleW, kTitleH })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_TEAL)
        .setText("AVG / R HR")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);



    mGlanceValueAHR = mGlanceUI.createText();

    mGlanceValueAHR
        .pos({ kValueAHRX, kValueAHRY }, { kValueAHRW, kValueAHRH })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER)
        .setText("---");



    mGlanceValueRHR = mGlanceUI.createText();

    mGlanceValueRHR
        .pos({ kValueRHRX, kValueRHRY }, { kValueRHRW, kValueRHRH })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER)
        .setText("---");
}
