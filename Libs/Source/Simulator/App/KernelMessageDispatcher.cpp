/**
 * @file   KernelMessageDispatcher.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Kernel-bound message dispatcher
 */

#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include <cstring>
#include <memory>

#define LOG_MODULE_PRX      "KMsgDispatcher"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace SDK::App
{

KernelMessageDispatcher::KernelMessageDispatcher(::App::MessageManager&      messageManager,
                                                 SDK::Interface::IVibro&     vibro,
                                                 SDK::Interface::IBacklight& backlight,
                                                 SDK::Interface::IBuzzer&    buzzer)
    : mMessageManager(messageManager)
    , mVibro(vibro)
	, mBacklight(backlight)
	, mBuzzer(buzzer)
    , mLocalSettings{}
{}

bool KernelMessageDispatcher::dispatchMessage(SDK::MessageBase* msg)
{
    appLifeCycleHandler(msg);
    appMsgHandler(msg);

	return true;
}

void KernelMessageDispatcher::appLifeCycleHandler(SDK::MessageBase* msg)
{
    if (!msg) {
        return;
    }

    //switch (msg->getType()) {
    //case App::InternalMessageType::APP_RUN_SRV: {
    //    bool status = runSrv();
    //    if (msg->getResult() == SDK::MessageResult::TIMEOUT) {
    //        // Should be stopped as we are not feet in time
    //        stopSrv();
    //    }
    //    else {
    //        msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
    //        mMessageManager.signalCompletion(msg);

    //        if (status && mLifeCycleCb && mBase) {
    //            mLifeCycleCb->onSrvStart(mBase);
    //        }
    //    }
    //} break;

    //case App::InternalMessageType::APP_RUN_SRV_ANG_GUI: {
    //    bool status = runSrv();
    //    if (status) {
    //        status = runGui();
    //        if (!status) {
    //            stopSrv();
    //        }
    //    }

    //    if (msg->getResult() == SDK::MessageResult::TIMEOUT) {
    //        // Should be stopped as we are not feet in time
    //        stopGui();
    //        stopSrv();
    //    }
    //    else {
    //        msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
    //        mMessageManager.signalCompletion(msg);

    //        if (status && mLifeCycleCb && mBase) {
    //            mLifeCycleCb->onSrvStart(mBase);
    //            mLifeCycleCb->onGuiStart(mBase);
    //        }
    //    }
    //} break;

    //case App::InternalMessageType::APP_STOP_SRV_ANG_GUI: {
    //    bool isGuiWasRun = isGuiRun();
    //    bool isSrvWasRun = isSrvRun();

    //    if (isGuiWasRun) {
    //        stopGui();
    //    }
    //    if (isSrvWasRun) {
    //        stopSrv();
    //    }

    //    if (!isSrvWasRun) {
    //        mFlags.set(Flag::Stopped);
    //    }

    //    mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);

    //    // Run timer for stop confirmation
    //    //TODO:
    //} break;

    //case App::InternalMessageType::APP_SUSPEND_GUI: {
    //    if (isGuiRun()) {
    //        bool wasResumed = isGuiResumed();
    //        suspendGui();
    //        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);

    //        if (wasResumed && mLifeCycleCb && mBase) {
    //            mLifeCycleCb->onGuiSuspend(mBase);
    //        }
    //    }
    //    else {
    //        mMessageManager.signalCompletion(msg, SDK::MessageResult::FAIL);
    //    }
    //} break;

    //case App::InternalMessageType::APP_RESUME_GUI: {
    //    if (isGuiRun()) {
    //        bool wasResumed = isGuiResumed();
    //        bool status = resumeGui();

    //        if (!wasResumed && status && msg->getResult() == SDK::MessageResult::TIMEOUT) {
    //            suspendGui();
    //            // no response needed as timeout occurs
    //        }
    //        else {
    //            msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
    //            mMessageManager.signalCompletion(msg);

    //            if (status && mLifeCycleCb && mBase) {
    //                mLifeCycleCb->onGuiResume(mBase);
    //            }
    //        }
    //    }
    //    else {
    //        mMessageManager.signalCompletion(msg, SDK::MessageResult::FAIL);
    //    }
    //} break;

    //case App::InternalMessageType::APP_GLANCE_RUN: {
    //    bool isSrvWasRun = isSrvRun();
    //    bool status = runSrv();
    //    if (status) {
    //        status = runGlc();
    //        if (!isSrvWasRun && !status) {
    //            // cleanup state
    //            stopSrv();
    //        }
    //    }

    //    msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
    //    mMessageManager.signalCompletion(msg);

    //    if (status && mLifeCycleCb && mBase) {
    //        if (!isSrvWasRun) {
    //            mLifeCycleCb->onSrvStart(mBase);
    //        }
    //        mLifeCycleCb->onGlanceRun(mBase);
    //    }
    //} break;

    //case App::InternalMessageType::APP_GLANCE_STOP: {
    //    if (isSrvRun()) {
    //        bool isGlanceWasRun = isGlanceRun();
    //        stopGlc();
    //        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);

    //        if (isGlanceWasRun && mLifeCycleCb && mBase) {
    //            mLifeCycleCb->onGlanceStop(mBase);
    //        }
    //    }
    //    else {
    //        mMessageManager.signalCompletion(msg, SDK::MessageResult::FAIL);
    //    }
    //} break;

    //case SDK::MessageType::REQUEST_APP_TERMINATE: {
    //    bool isGuiStopped = false;
    //    bool isSrvStopped = false;

    //    auto* terminateMsg = static_cast<SDK::Message::RequestAppTerminate*>(msg);
    //    if (terminateMsg->getProcessId() == mInfo.srvPID) {
    //        // Terminate GUI first
    //        if (mGuiLoader.isLoaded()) {
    //            mGuiLoader.unload();
    //            mGuiKernel.reset();
    //            mGuiState = AppPartState::NONE;
    //            mGuiTickNumber = 0;
    //            LOG_DEBUG("Process with PID 0x%08X terminated\n", mInfo.name, mInfo.guiPID);
    //            isGuiStopped = true;
    //        }

    //        if (mSrvLoader.isLoaded()) {
    //            mSrvLoader.unload();
    //            mSrvKernel.reset();
    //            mSrvState = AppPartState::NONE;
    //            mGlanceTickNumber = 0;
    //            LOG_DEBUG("Process with PID 0x%08X terminated\n", mInfo.name, mInfo.srvPID);
    //            isSrvStopped = true;

    //        }
    //    }
    //    else {
    //        if (mGuiLoader.isLoaded()) {
    //            bool isGuiWasRun = isGuiRun();

    //            mGuiLoader.unload();
    //            mGuiKernel.reset();
    //            mGuiState = AppPartState::NONE;
    //            mGuiTickNumber = 0;
    //            LOG_DEBUG("Process with PID 0x%08X terminated\n", mInfo.name, mInfo.guiPID);
    //            isGuiStopped = true;

    //            if (isGuiWasRun) {
    //                // Notify the service
    //                auto* notif = mMessageManager.allocateMessage<SDK::Message::CommandAppNotifGuiStop>();
    //                if (notif) {
    //                    mAppComm.sendToService(notif);
    //                    mMessageManager.releaseMessage(notif);
    //                }
    //            }
    //        }
    //    }
    //    mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);

    //    if (mLifeCycleCb && mBase) {
    //        if (isGuiStopped) {
    //            LOG_DEBUG("mLifeCycleCb->onGuiStop(mBase)\n");
    //            mLifeCycleCb->onGuiStop(mBase);
    //        }
    //        if (isSrvStopped) {
    //            mLifeCycleCb->onSrvStop(mBase);
    //        }
    //    }

    //    // Finally send stop event
    //    if (isSrvStopped) {
    //        mFlags.set(Flag::Stopped);
    //    }
    //} break;

    //case SDK::MessageType::REQUEST_APP_RUN_GUI: {
    //    LOG_DEBUG("Service requests GUI launch\n");
    //    bool isGuiWasRun = isGuiRun();
    //    bool status = runGui();
    //    msg->setResult(status ? SDK::MessageResult::SUCCESS : SDK::MessageResult::FAIL);
    //    mMessageManager.signalCompletion(msg);

    //    if (!isGuiWasRun && status && mLifeCycleCb && mBase) {
    //        mLifeCycleCb->onGuiStart(mBase);

    //        // Since the service launched the GUI on its own,
    //        // we need the kernel to allow context switching to this application.
    //        mLifeCycleCb->onGuiResumeRequest(mBase);
    //    }
    //} break;

    //case SDK::MessageType::REQUEST_SET_CAPABILITIES: {
    //    LOG_DEBUG("Sets capabilities\n");
    //    auto* caps = static_cast<SDK::Message::RequestSetCapabilities*>(msg);

    //    // Safety saving
    //    {
    //        OS::MutexCS cs(mMutex);
    //        mCapabilities.enPhoneNotification = caps->enPhoneNotification;
    //        mCapabilities.enUsbChargingScreen = caps->enUsbChargingScreen;
    //        mCapabilities.enMusicControl = caps->enMusicControl;
    //    }

    //    mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);

    //    if (mLifeCycleCb && mBase) {
    //        mLifeCycleCb->onCapabilities(mBase, mCapabilities);
    //    }
    //} break;

    //case SDK::MessageType::REQUEST_APP_NEW_ACTIVITY: {
    //    LOG_DEBUG("Sets new activity\n");
    //    if (mLifeCycleCb && mBase) {
    //        mLifeCycleCb->onNewActivity(mBase);
    //    }
    //} break;

    //default:
    //    break;
    //}
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

        settings->heartRateCount = 0;
        for (uint32_t i = 0; i < SDK::Message::RequestSystemSettings::skMaxHearRateTh; i++) {
            settings->heartRateTh[i] = mSettings.heartRateZones.thresholds[i];
            settings->heartRateCount++;
            if (i == HeartRateZones::kMaxThreshold) {
                break;
            }
        }

        settings->activityMin = mSettings.dailyGoals.activityMinutes;
        settings->steps = mSettings.dailyGoals.steps;
        settings->floors = mSettings.dailyGoals.floors;

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_DISPLAY_CONFIG: {
        LOG_DEBUG("Requests system settings\n");
        auto* displayCfgMsg = static_cast<SDK::Message::RequestDisplayConfig*>(msg);
        displayCfgMsg->width      = 240;
        displayCfgMsg->height     = 240;
        displayCfgMsg->colorDepth = 6;
        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_DISPLAY_UPDATE: {
        LOG_DEBUG("Requests display update\n");
        auto* displayUpdMsg = static_cast<SDK::Message::RequestDisplayUpdate*>(msg);
        //mLcd.write(displayUpdMsg->pBuffer);
        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
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

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_BUZZER_PLAY: {
        LOG_DEBUG("Requests buzzer\n");

        if (!mLocalSettings.alertMutedFlag) { // Alerts from GUI
            auto* buzzMsg = static_cast<SDK::Message::RequestBuzzerPlay*>(msg);

            if (buzzMsg->notesCount) {
				auto melody = std::make_unique<Interface::IBuzzer::Note[]>(buzzMsg->notesCount);
                for (uint8_t i = 0; i < buzzMsg->notesCount; i++) {
                    melody[i].level = (buzzMsg->notes[i].volume / 33);
                    melody[i].time = buzzMsg->notes[i].time;
                }
                mBuzzer.play(melody.get(), buzzMsg->notesCount);
            } else {
                mBuzzer.play();
            }
        }
        else {
            LOG_DEBUG("Ignore. Alerts muted\n");
        }

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_VIBRO_PLAY: {
        LOG_DEBUG("Requests vibro\n");

        if (!mLocalSettings.alertMutedFlag) { // Alerts from GUI
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

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_GLANCE_CONFIG: {
        LOG_DEBUG("Requests glance config\n");
        auto* gl = static_cast<SDK::Message::RequestGlanceConfig*>(msg);

        gl->width       = GLANCE_WIDTH;
        gl->height      = GLANCE_HEIGHT;
        gl->maxControls = GLANCE_MAX_CONTROLS;

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    case SDK::MessageType::REQUEST_GLANCE_UPDATE: {
        LOG_DEBUG("Requests glance update\n");
        auto* gl = static_cast<SDK::Message::RequestGlanceUpdate*>(msg);

        // Safety saving
        {
            //mGlanceData.count = 0;
            //if (gl->name) {
            //    snprintf(mGlanceData.altname, sizeof(mGlanceData.altname), "%s", gl->name);
            //}
            //else {
            //    snprintf(mGlanceData.altname, sizeof(mGlanceData.altname), "%s", mInfo.name);
            //}
            //for (uint32_t i = 0; i < gl->controlsNumber; i++) {
            //    LOG_DEBUG("ctrl %d, id %u, valid %d\n", i, gl->controls[i].id, gl->controls[i].valid);
            //    mGlanceData.ctrls[i] = gl->controls[i];
            //    mGlanceData.count++;
            //    if (mGlanceData.count == GLANCE_MAX_CONTROLS) {
            //        LOG_DEBUG("Maximum number of glance controls reached\n");
            //        break;
            //    }
            //}
        }

        mMessageManager.signalCompletion(msg, SDK::MessageResult::SUCCESS);
    } break;

    default:
        break;
    }
}


} // namespace SDK::App

