/**
 * @file   KernelMessageDispatcher.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Kernel-bound message dispatcher
 */

#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Simulator/Components/SensorManager.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Simulator/Components/InstanceSensorLayer.hpp"

#include <cstring>
#include <memory>

#define LOG_MODULE_PRX      "KMsgDispatcher"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"
#include <SDK/Simulator/Kernel/Mock/System.hpp>

#define ARRAY_SIZE(A)           (sizeof(A) / sizeof(A[0]))

namespace SDK::App
{

KernelMessageDispatcher::KernelMessageDispatcher(SDK::App::DualAppComm& appComm,
                                                ::App::MessageManager& messageManager,
                                                SDK::Interface::IVibro& vibro,
                                                SDK::Interface::IBacklight& backlight,
                                                SDK::Interface::IBuzzer& buzzer)
    : mAppComm(appComm)
    , mMessageMgr(messageManager)
    , mVibro(vibro)
	, mBacklight(backlight)
	, mBuzzer(buzzer)
    , mLocalSettings{}
    , mSrvSensorListener(mAppComm, true)
{}

void KernelMessageDispatcher::run()
{
    Instance::SensorLayer::getInstance().init();

    while (1) {
        // Wait for message (blocks until available)
        SDK::MessageBase* msg = nullptr;
        mAppComm.receiveFromApp(msg, 500);

        if (!SDK::Simulator::Mock::SystemGUI::isAppRunning()) {
            return;
        }

        if (msg == nullptr) {
            LOG_DEBUG("msg == nullptr\n");
            continue;
        }

        LOG_DEBUG("Message 0x%08X received \n", msg->getType());

        // Dispatch
        appLifeCycleHandler(msg);
        appMsgHandler(msg);
        slMsgHandler(msg);

        // Default handler
        if (msg->getResult() == SDK::MessageResult::PENDING) {
            msg->setResult(SDK::MessageResult::FAIL);
            mMessageMgr.signalCompletion(msg);
        }

        // Release message
        mMessageMgr.releaseMessage(msg);
        LOG_DEBUG("Message 0x%08X released \n", msg->getType());
    }
}

void KernelMessageDispatcher::appLifeCycleHandler(SDK::MessageBase* msg)
{
    if (!msg) {
        return;
    }

    switch (msg->getType()) {
        case SDK::MessageType::REQUEST_APP_TERMINATE: {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_APP_RUN_GUI: {
            LOG_DEBUG("Service requests GUI launch\n");
            bool isGuiWasRun = true;
            bool status = true;
            msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
            mMessageMgr.signalCompletion(msg);
        } break;

        case SDK::MessageType::REQUEST_SET_CAPABILITIES: {
            LOG_DEBUG("Sets capabilities\n");
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);

        } break;

        case SDK::MessageType::REQUEST_APP_NEW_ACTIVITY: {
            LOG_DEBUG("Sets new activity\n");
        } break;

        default:
            break;
    }
}

void KernelMessageDispatcher::appMsgHandler(SDK::MessageBase* msg)
{
    if (!msg) {
        return;
    }

    switch (msg->getType()) {
        case SDK::MessageType::REQUEST_SYSTEM_SETTINGS: {
            LOG_DEBUG("Requests system settings\n");
            auto* settings = static_cast<SDK::Message::RequestSystemSettings*>(msg);

            settings->imperialUnits = mSettings.unitsImperial;

            const uint32_t limit = std::min(
                SDK::Message::RequestSystemSettings::skMaxHearRateTh,
                static_cast<const uint32_t>(HeartRateZones::kMaxThreshold)
            );

            for (uint32_t i = 0; i < limit; i++) {
                settings->heartRateTh[i] = mSettings.heartRateZones.thresholds[i];
            }

            settings->heartRateCount = limit;

            settings->activityMin = mSettings.dailyGoals.activityMinutes;
            settings->steps = mSettings.dailyGoals.steps;
            settings->floors = mSettings.dailyGoals.floors;

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_DISPLAY_CONFIG: {
            LOG_DEBUG("Requests system settings\n");
            auto* displayCfgMsg = static_cast<SDK::Message::RequestDisplayConfig*>(msg);
            displayCfgMsg->width = 240;
            displayCfgMsg->height = 240;
            displayCfgMsg->colorDepth = 6;
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_DISPLAY_UPDATE: {
            LOG_DEBUG("Requests display update\n");

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_BACKLIGHT_SET: {
            LOG_DEBUG("Requests backlight\n");
            auto* blMsg = static_cast<SDK::Message::RequestBacklightSet*>(msg);

            if (blMsg->brightness > 0) {
                mBacklight.on(blMsg->autoOffTimeoutMs);
            }
            else {
                mBacklight.off();
            }

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_BUZZER_PLAY: {
            LOG_DEBUG("Requests buzzer\n");

            if (!mLocalSettings.alertMutedFlag) {
                //Alerts from GUI
                    auto* buzzMsg = static_cast<SDK::Message::RequestBuzzerPlay*>(msg);

                if (buzzMsg->notesCount) {
                    auto melody = std::make_unique<Interface::IBuzzer::Note[]>(buzzMsg->notesCount);
                    for (uint8_t i = 0; i < buzzMsg->notesCount; i++) {
                        melody[i].level = (buzzMsg->notes[i].volume / 33);
                        melody[i].time = buzzMsg->notes[i].time;
                    }
                    mBuzzer.play(melody.get(), buzzMsg->notesCount);
                }
                else {
                    mBuzzer.play();
                }
            }
            else {
                LOG_DEBUG("Ignore. Alerts muted\n");
            }

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_VIBRO_PLAY: {
            LOG_DEBUG("Requests vibro\n");

            if (!mLocalSettings.alertMutedFlag) {
                //Alerts from GUI
                    auto* vibroMsg = static_cast<SDK::Message::RequestVibroPlay*>(msg);

                if (vibroMsg->notesCount) {
                    auto melody = std::make_unique<Interface::IVibro::Note[]>(vibroMsg->notesCount);

                    for (uint8_t i = 0; i < vibroMsg->notesCount; i++) {
                        melody[i].effect = vibroMsg->notes[i].effect;
                        melody[i].loop = 0;
                        melody[i].pause = vibroMsg->notes[i].pause;
                    }
                    mVibro.play(melody.get(), vibroMsg->notesCount);
			    } else {
                    mVibro.play();
                }
            }
            else {
                LOG_DEBUG("Ignore. Alerts muted\n");
            }

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_GLANCE_CONFIG: {
            LOG_DEBUG("Requests glance config\n");
            auto* gl = static_cast<SDK::Message::RequestGlanceConfig*>(msg);

            gl->width       = GLANCE_WIDTH;
            gl->height      = GLANCE_HEIGHT;
            gl->maxControls = GLANCE_MAX_CONTROLS;

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_GLANCE_UPDATE: {
            LOG_DEBUG("Requests glance update\n");
            auto* gl = static_cast<SDK::Message::RequestGlanceUpdate*>(msg);

            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_WIDGET_START: {
            LOG_DEBUG("Widget start\n");
            // No home screen to render in the simulator; just accept it.
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_WIDGET_UPDATE: {
            LOG_DEBUG("Widget update\n");
            // No home screen to render in the simulator; just accept it.
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        case SDK::MessageType::REQUEST_WIDGET_STOP: {
            LOG_DEBUG("Widget stop\n");
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        } break;

        default:
            break;
    }
}

void KernelMessageDispatcher::slMsgHandler(SDK::MessageBase* msg)
{
    if (!msg) {
        return;
    }

    switch (msg->getType()) {

    case SDK::MessageType::REQUEST_SENSOR_LAYER_GET_DEFAULT: {
        LOG_DEBUG("Requests get default sensor\n");
        auto* req = static_cast<SDK::Message::Sensor::RequestDefault*>(msg);
        auto* driver = ::Sensor::Manager::getInstance().getDefaultSensor(req->id);
        if (driver) {
            req->handle = driver->getHandle();
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        }
        else {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::FAIL);
        }
    } break;

    case SDK::MessageType::REQUEST_SENSOR_LAYER_GET_LIST: {
        LOG_DEBUG("Requests get sensor list\n");
        auto* req = static_cast<SDK::Message::Sensor::RequestList*>(msg);
        auto  list = ::Sensor::Manager::getInstance().getHandleList(req->id);
        if (list.size() <= ARRAY_SIZE(req->handles)) {
            req->handlesCount = list.size();
            for (uint8_t idx = 0; idx < list.size(); ++idx) {
                req->handles[idx] = list[idx];
            }
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        }
        else {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::FAIL);
        }
    } break;

    case SDK::MessageType::REQUEST_SENSOR_LAYER_GET_DESCRIPTOR: {
        LOG_DEBUG("Requests get sensor description\n");
        auto* req = static_cast<SDK::Message::Sensor::RequestGetDesc*>(msg);
        auto* driver = ::Sensor::Manager::getInstance().getDriverByHandle(req->handle);
        if (driver) {
            strncpy(req->desc, driver->getDescription(), sizeof(req->desc));
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        }
        else {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::FAIL);
        }
    } break;

    case SDK::MessageType::REQUEST_SENSOR_LAYER_CONNECT: {
        LOG_DEBUG("Requests connect to sensor\n");
        auto* req = static_cast<SDK::Message::Sensor::RequestConnect*>(msg);
        auto* driver = ::Sensor::Manager::getInstance().getDriverByHandle(req->handle);

        SDK::Interface::ISensorDataListener* listener;
        listener = &mSrvSensorListener;

        if (driver && listener && driver->connect(listener, req->period, req->latency)) {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        }
        else {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::FAIL);
        }
    } break;

    case SDK::MessageType::REQUEST_SENSOR_LAYER_DISCONNECT: {
        LOG_DEBUG("Requests disconnect from sensor\n");
        auto* req = static_cast<SDK::Message::Sensor::RequestDisconnect*>(msg);
        auto* driver = ::Sensor::Manager::getInstance().getDriverByHandle(req->handle);

        SDK::Interface::ISensorDataListener* listener;
        listener = &mSrvSensorListener;

        if (driver && listener) {
            driver->disconnect(listener);
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::SUCCESS);
        }
        else {
            mMessageMgr.signalCompletion(msg, SDK::MessageResult::FAIL);
        }
    } break;

    default:
        break;
    }


}

} // namespace SDK::App

