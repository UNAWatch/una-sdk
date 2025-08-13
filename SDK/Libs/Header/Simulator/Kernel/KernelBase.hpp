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

#ifndef __SIMULATOR_KERNEL_BASE_HPP
#define __SIMULATOR_KERNEL_BASE_HPP

#include <cstdint>
#include <cassert>

#include "IKernel.hpp"

#include "MockPower.hpp"
#include "MockTime.hpp"
#include "MockSettings.hpp"
#include "MockActivity.hpp"
#include "FileSystem.hpp"
#include "MockUserAppMemAllocator.hpp"
#include "SynchManager/SynchManager.hpp"
#include "MockUserApp.hpp"
#include "MockServiceControl.hpp"
#include "BacklightStub.hpp"
#include "BuzzerStub.hpp"
#include "VibroStub.hpp"
#include "ISensorCore.hpp"

namespace Simulator
{

/**
 * @class KernelBase
 * 
 * This class initializes the 'app <-> kernel' interface (IKernel), and
 * performs the basic operations required for the application to function.
 */
class KernelBase {
public:

    KernelBase(bool useMutex, MockServiceControl& serviceControl, Interface::ISensorCore* sensoreCore = nullptr);

    virtual ~KernelBase() = default;

    /**
     * @brief   The function should be called to execute a series of callbacks
     *          to the user application during its loading and startup:
     *          'onCreate()' -> 'onStart()' -> 'onResume()'
     */
    virtual void startApp();

    /**
     * @brief   The function should be called to execute a series of callbacks
     *          The function should be called to execute a series of callbacks
     *          to the user application when it completes:
     *          'onPause()' -> 'onStop()' -> 'onDestroy()'
     */
    virtual void stopApp();

    /**
     * @brief   The function is called synchronously before the model tick and can
     *          be used for kernel simulator logic.
     */
    virtual void tick();

    /**
     * @brief   This function allows you to send to the Model and Screens only
     *          key presses that correspond to the physical buttons of the device.
     *          Other keys can be used to control the simulator.
     * @param   key: Detected key value (if any).
     * @retval  'true' - if the pressed key symbol should be passed to the Model or
     *          to the Screen, 'false' - otherwise.
     */
    virtual bool keyFilter(uint8_t key);

    const IKernel* getIKernel();

protected:
    MockPower               mIPower;
    MockTime                mITime;
    MockSettings            mISettings;
    MockActivity            mIActivity;
    FileSystem              mIFilesystem;
    MockUserAppMemAllocator mIUserAppMemAllocator;
    OS::SynchManager        mSynchManager;
    MockUserApp             mIUserApp;
    MockServiceControl&     mServiceControl;
    Stub::Backlight         mBacklight;
    Stub::Buzzer            mBuzer;
    Stub::Vibro             mVibro;
    Interface::ISensorCore* mSensoreCore;

    /**
     * @brief   The function initializes the "app <-> kernel" interface (IKernel).
     * @note    This function is called via KernelHolder::Create(). It must be 
     *          called before the first kernel access.
     */
    virtual void initInterface();

private:

    friend class KernelHolder;

    const IKernel* mKernel;
};


/**
 * @class KernelHolder
 * 
 * Helper class for storing a pointer to a kernel object.
 */
class KernelHolder {
public:
    static void Create(KernelBase &kernel)
    {
        instance() = &kernel;
        kernel.initInterface();
    }

    static KernelBase &Get()
    {
        assert(instance() != nullptr);
        return *instance();
    }
private:
    static KernelBase *&instance()
    {
        static KernelBase *pKernel;
        return pKernel;
    }
};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_BASE_HPP */