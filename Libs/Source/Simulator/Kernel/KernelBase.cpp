/**
 ******************************************************************************
 * @file    KernelBase.hpp
 * @date    05-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   The base class of the kernel simulator.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Kernel/KernelBase.hpp"
#include "SDK/Simulator/Kernel/Mock/ServiceControl.hpp"
#include "SDK/Simulator/Sensors/ISensorCore.hpp"
#include "SDK/Interfaces/IKernelIntfProvider.hpp"

static constexpr char sFsPath[] = "../../../../../Output/";

namespace SDK::Simulator
{

KernelBase::KernelBase(Mock::ServiceControl& serviceControl,
    Sensors::ISensorCore* sensoreCore,
    Mock::App* srvApp)
    : mIsServise(srvApp == nullptr) , mSrvApp(srvApp) , mSensoreCore(sensoreCore), mAppMutex()
    // Init Mocks
    , mISystem(mIsServise ? &mAppMutex : nullptr), mILogger()
    , mIAppMemory(), mIApp(mIsServise ? &mAppMutex : nullptr), mIAppCapabilities()
    , mSynchManager(), mServiceControl(serviceControl), mIPower(), mISettings()
    , mIFilesystem(sFsPath), mIBacklight(), mIVibro(), mIBuzer(), mITime(), mSensorManager()
    // Init pointers to the mocks
    , isystem(&mISystem), ilogger(&mILogger), imem(&mIAppMemory), iappmock(&mIApp)
    , iappCapabilities(&mIAppCapabilities), isynchManager(&mSynchManager)
    , isctrl(&mServiceControl), igctrl(&mServiceControl), ipwr(&mIPower)
    , isettings(&mISettings), ifs(&mIFilesystem), ibacklight(&mIBacklight)
    , ivibro(&mIVibro), ibuzzer(&mIBuzer), itime(&mITime), isensorManager(&mSensorManager)
    // Init KIP with references
    , mKip(*this)
    // Init base class with KIP
    , IKernel(mKip)
{
}

void KernelBase::startApp()
{
    iappmock->create();
    iappmock->start();
    if (mSrvApp) {
        mSrvApp->guiState(true);
    }
    iappmock->resume();
}

void KernelBase::stopApp()
{
    iappmock->pause();
    if (mSrvApp) {
        mSrvApp->guiState(false);
    }
    iappmock->stop();
    iappmock->destroy();
}

void KernelBase::tick()
{
    if (mSensoreCore) {
        mSensoreCore->tick();
    }
}

bool KernelBase::keyFilter(uint8_t key)
{
    return (!mIsServise && iappmock->getState() == Mock::App::State::RESUMED &&
            (SDK::Interface::IApp::Button::BUTTON_L1 == key ||
             SDK::Interface::IApp::Button::BUTTON_L2 == key ||
             SDK::Interface::IApp::Button::BUTTON_R1 == key ||
             SDK::Interface::IApp::Button::BUTTON_R2 == key ||
             SDK::Interface::IApp::Button::BUTTON_L1R2 == key));

}

Mock::App& KernelBase::getApp()
{
    return *iappmock;
}

std::string KernelBase::getFsPath()
{
    return mIFilesystem.getRootPath();
}

} // namespace SDK::Simulator