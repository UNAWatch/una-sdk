/**
 ******************************************************************************
 * @file    Kernel.hpp
 * @date    05-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   The base class of the kernel simulator.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IKIP.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Kernel/Mock/AppMemory.hpp"
#include "SDK/Simulator/Kernel/Mock/FileSystem.hpp"
#include "SDK/Simulator/Kernel/Mock/Logger.hpp"
#include "SDK/Simulator/Sensors/ISensorCore.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <memory>


namespace SDK::Simulator
{

/**
 * @class Kernel
 * 
 * This class initializes the 'app <-> kernel' interface (IKernel), and
 * performs the basic operations required for the application to function.
 */
class Kernel
{
public:

    /**
     * @brief   Constructor.
     * @param   serviceControl: Reference to the service control object.
     * @param   sensoreCore: Pointer to the sensor core interface (optional).
     * @param   srvApp: Pointer to the Service App (only for GUI). 
     *          If not 'nullptr', the kernel will use mutexes for synchronization.
     */
    Kernel(const char* name, SDK::Simulator::Sensors::ISensorCore* sensoreCore = nullptr);
    ~Kernel() = default;

    SDK::Kernel& getKernel();

    void markAsStopped()
    {
        mKernelIsStopped = true;
    }

    /**
     * @brief   The function is called synchronously before the model tick and can
     *          be used for kernel simulator logic.
     */
    void tick();

    /**
     * @brief   This function allows you to send to the Model and Screens only
     *          key presses that correspond to the physical buttons of the device.
     *          Other keys can be used to control the simulator.
     * @param   key: Detected key value (if any).
     * @retval  'true' - if the pressed key symbol should be passed to the Model or
     *          to the Screen, 'false' - otherwise.
     */
    bool keyFilter(uint8_t key);

    std::string getFsPath();

    void setISystem(SDK::Interface::ISystem* isystem);
    void setILogger(SDK::Interface::ILogger* ilogger);
    void setIAppMemory(SDK::Interface::IAppMemory* imem);
    void setIAppComm(SDK::Interface::IAppComm* comm);
    void setIFileSystem(SDK::Interface::IFileSystem* ifs);

protected:
    Sensors::ISensorCore* mSensoreCore;
    const bool            mIsServise;

private:
    void* queryInterface(SDK::Interface::IKIP::IntfID iid);

    const char* mName;

    Mock::Logger     mLogger;
    Mock::AppMemory  mAppMemory;
    Mock::FileSystem mFilesystem;

    SDK::Interface::ISystem*      mISystem;
    SDK::Interface::ILogger*      mILogger;
    SDK::Interface::IAppMemory*   mIAppMemory;
    SDK::Interface::IAppComm*     mIAppComm;
    SDK::Interface::IFileSystem*  mIFilesystem;

    std::unique_ptr<SDK::Kernel> mKernel;

	volatile bool mKernelIsStopped;
};


/**
 * @class KernelHolder
 * 
 * Helper class for storing a pointer to a kernel object for simulator.
 */
class KernelHolder {
public:
    /**
     * @brief   Create a kernel instance and initialize the interface.
     * @param   kernel: The kernel instance to be created.
     */
    static void Create(Kernel &kernel)
    {
        instance() = &kernel;
    }

    /**
     * @brief Get the kernel instance.
     */
    static Kernel& Get()
    {
        assert(instance() != nullptr);
        
        return *instance();
    }

private:
    
    /**
     * @brief   Get the pointer to the kernel instance.
     * @note    This function is used to access the kernel instance.
     */
    static Kernel*& instance()
    {
        static Kernel *pKernel;
        return pKernel;
    }
};

} // namespace SDK::Simulator
