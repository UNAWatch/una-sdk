#include "Service.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"

#include "IconsBattery.h"

Service::Service(SDK::Kernel &kernel)
    : mKernel(kernel)
    , mGlanceUI()
    , mGlanceTitle()
    , mGlanceValue()
    , mIcon()
    , mSensorSOC(SDK::Sensor::Type::BATTERY_LEVEL)
    , mSOCValue(0)
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
    if (!mSensorSOC.isConnected()) {
        LOG_DEBUG("Connect to sensors...\n");
        mSensorSOC.connect();
    }
}

void Service::disconnect()
{
    if (mSensorSOC.isConnected()) {
        LOG_DEBUG("Disconnect from sensors...\n");
        mSensorSOC.disconnect();
    }
}

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorSOC.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel p(data[0]);

        if (!p.isDataValid()) {
            return;
        }

        float newValue = p.getCharge();

        LOG_DEBUG("Level: %.1f\n", newValue);

        if (static_cast<uint32_t>(newValue) != mSOCValue || !mDataReceived) {
            mDataReceived = true;
            mSOCValue = static_cast<uint32_t>(newValue);
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
    mGlanceValue.print("%u%%", mSOCValue);

    /**
     *  Level     s1          s2      s3      s4
     *  0 %       off         off     off     off
     *  1-24 %    red         off     off     off
     *  25-49 %   teal        teal    off     off
     *  50-74 %   teal        teal    teal    off
     *  75-100 %  teal        teal    teal    teal
     */

    if (mSOCValue >= 75) {
        mIcon.setImage(ICON_BATTERY_100_ABGR2222);
    } else if (mSOCValue >= 50) {
        mIcon.setImage(ICON_BATTERY_75_ABGR2222);
    } else if (mSOCValue >= 25) {
        mIcon.setImage(ICON_BATTERY_50_ABGR2222);
    } else if (mSOCValue >= 1) {
        mIcon.setImage(ICON_BATTERY_25_ABGR2222);
    } else {
        mIcon.setImage(ICON_BATTERY_0_ABGR2222);
    }
}

void Service::createGuiControls()
{
    mIcon = mGlanceUI.createImage();
    mIcon.init({kIconX, kIconY}, {ICON_BATTERY_WIDTH, ICON_BATTERY_HEIGHT}, ICON_BATTERY_0_ABGR2222);

    mGlanceTitle = mGlanceUI.createText();
    mGlanceTitle.pos({ kTitleX, kTitleY }, { kTitleW, kTitleH })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_TEAL)
        .setText("Battery Level")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    mGlanceValue = mGlanceUI.createText();
    mGlanceValue.pos({ kValueX, kValueY }, { kValueW, kValueH })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .setText("---")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);
}
