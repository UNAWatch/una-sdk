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

#include "Simulator/Kernel/Mock/Power.hpp"
#include "Simulator/Kernel/Mock/Settings.hpp"
#include "Simulator/Kernel/Mock/FileSystem.hpp"
#include "Simulator/Kernel/Mock/SensorManager.hpp"
#include "Simulator/Kernel/Mock/Backlight.hpp"
#include "Simulator/Kernel/Mock/Buzzer.hpp"
#include "Simulator/Kernel/Mock/Vibro.hpp"

#include "SynchManager/SynchManager.hpp"
#include "Simulator/Kernel/Mock/MockUserAppMemAllocator.hpp"
#include "Simulator/Kernel/Mock/MockTime.hpp"
#include "Simulator/Kernel/Mock/MockUserApp.hpp"
#include "Simulator/Kernel/Mock/MockServiceControl.hpp"

#include "Simulator/Sensors/ISensorCore.hpp"

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

    /**
     * @brief   Constructor.
     * @param   useMutex: If 'true', the kernel will use mutexes for synchronization.
     * @param   serviceControl: Reference to the service control object.
     * @param   sensoreCore: Pointer to the sensor core interface (optional).
     */
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

    /**
     * @brief   Get the pointer to the kernel interface.
     * @retval  Pointer to the kernel interface (IKernel).
     */
    const IKernel* getIKernel();

protected:
    Mock::Power             mIPower;
    MockTime                mITime;
    Mock::Settings          mISettings;
    Mock::FileSystem        mIFilesystem;
    MockUserAppMemAllocator mIUserAppMemAllocator;
    OS::SynchManager        mSynchManager;
    Mock::SensorManager     mSensorManager;
    MockUserApp             mIUserApp;
    MockServiceControl&     mServiceControl;
    Mock::Backlight         mBacklight;
    Mock::Buzzer            mBuzer;
    Mock::Vibro             mVibro;
    Interface::ISensorCore* mSensoreCore;

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
    /**
     * @brief   Create a kernel instance and initialize the interface.
     * @param   kernel: The kernel instance to be created.
     */
    static void Create(KernelBase &kernel)
    {
        instance() = &kernel;
    }

    /**
     * @brief   Get the kernel instance.
     */
    static KernelBase &Get()
    {
        assert(instance() != nullptr);
        return *instance();
    }
private:
    
    /**
     * @brief   Get the pointer to the kernel instance.
     * @note    This function is used to access the kernel instance.
     */
    static KernelBase *&instance()
    {
        static KernelBase *pKernel;
        return pKernel;
    }
};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_BASE_HPP */