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

#include "KernelBase.hpp"
#include "gui/common/GuiConfig.hpp"
#include "Simulator/Kernel/Mock/MockServiceControl.hpp"
#include "Simulator/Sensors/ISensorCore.hpp"

namespace Simulator
{
KernelBase::KernelBase(bool useMutex, MockServiceControl& serviceControl, Interface::ISensorCore* sensoreCore)
    : mIPower()
    , mITime()
    , mISettings()
    , mIFilesystem("../../../../../Output/")
    , mIUserAppMemAllocator()
    , mSynchManager()
    , mIUserApp(useMutex)
    , mServiceControl(serviceControl)
    , mBacklight()
    , mBuzer()
    , mVibro()
    , mSensoreCore(sensoreCore)
    , mKernel(new IKernel(mIPower,
                          mITime,
                          mISettings,
                          mIFilesystem,
                          mIUserAppMemAllocator,
                          mSynchManager,
                          mSensorManager,
                          mIUserApp,
                          mServiceControl,
                          mServiceControl,
                          mBacklight,
                          mVibro,
                          mBuzer))
{
}

void KernelBase::startApp()
{ 
    mIUserApp.create();
    mIUserApp.start();
    mIUserApp.resume();
}

void KernelBase::stopApp()
{
    mIUserApp.pause();
    mIUserApp.stop();
    mIUserApp.destroy();
}

void KernelBase::tick()
{
    if (mSensoreCore) {
        mSensoreCore->tick();
    }
}

bool KernelBase::keyFilter(uint8_t key)
{
    return (mIUserApp.getState() == MockUserApp::State::RESUMED &&
            (Gui::Config::Button::L1 == key ||
             Gui::Config::Button::L2 == key ||
             Gui::Config::Button::R1 == key ||
             Gui::Config::Button::R2 == key ||
             Gui::Config::Button::L1R2 == key));
}

const IKernel* KernelBase::getIKernel()
{
    return mKernel;
}

} /* namespace Simulator */
