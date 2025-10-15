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

#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#include "SDK/Interfaces/IKernel.hpp"

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SynchManager/SynchManager.hpp"

#include "SDK/Simulator/Kernel/Mock/App.hpp"
#include "SDK/Simulator/Kernel/Mock/AppCapabilities.hpp"
#include "SDK/Simulator/Kernel/Mock/AppMemory.hpp"
#include "SDK/Simulator/Kernel/Mock/Backlight.hpp"
#include "SDK/Simulator/Kernel/Mock/Buzzer.hpp"
#include "SDK/Simulator/Kernel/Mock/FileSystem.hpp"
#include "SDK/Simulator/Kernel/Mock/Logger.hpp"
#include "SDK/Simulator/Kernel/Mock/Power.hpp"
#include "SDK/Simulator/Kernel/Mock/SensorManager.hpp"
#include "SDK/Simulator/Kernel/Mock/ServiceControl.hpp"
#include "SDK/Simulator/Kernel/Mock/Settings.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include "SDK/Simulator/Kernel/Mock/Time.hpp"
#include "SDK/Simulator/Kernel/Mock/Vibro.hpp"

#include "SDK/Simulator/Sensors/ISensorCore.hpp"

namespace SDK::Simulator
{

/**
 * @class KernelBase
 * 
 * This class initializes the 'app <-> kernel' interface (IKernel), and
 * performs the basic operations required for the application to function.
 */
class KernelBase : public SDK::Interface::IKernel {
public:

    /**
     * @brief   Constructor.
     * @param   serviceControl: Reference to the service control object.
     * @param   sensoreCore: Pointer to the sensor core interface (optional).
     * @param   srvApp: Pointer to the Service App (only for GUI). 
     *          If not 'nullptr', the kernel will use mutexes for synchronization.
     */
    KernelBase(Mock::ServiceControl& serviceControl, 
               SDK::Simulator::Sensors::ISensorCore* sensoreCore = nullptr,
               Mock::App* srvApp = nullptr);

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

    Mock::App& getApp();

    std::string getFsPath();


    void setISystem(SDK::Interface::ISystem* isystem) {
        if (isystem) { this->isystem = isystem; }
    }

    void setILogger(SDK::Interface::ILogger* ilogger) {
        if (ilogger) { this->ilogger = ilogger; }
    }

    void setIAppMemory(SDK::Interface::IAppMemory* imem) {
        if (imem) { this->imem = imem; }
    }

    void setIApp(SDK::Simulator::Mock::App* iappmock) {
        if (iappmock) { this->iappmock = iappmock; }
    }

    void setIAppCapabilities(SDK::Interface::IAppCapabilities* iappCapabilities) {
        if (iappCapabilities) { this->iappCapabilities = iappCapabilities; }
    }

    void setISynchManager(SDK::Interface::ISynchManager* isynchManager) {
        if (isynchManager) { this->isynchManager = isynchManager; }
    }

    void setIServiceControl(SDK::Interface::IServiceControl* isctrl) { 
        if (isctrl) { this->isctrl = isctrl; }
    }

    void setIGUIControl(SDK::Interface::IGUIControl* isctrl) {
        if (igctrl) { this->igctrl = igctrl; }
    }

    void setIPower(SDK::Interface::IPower* ipwr) {
        if (ipwr) { this->ipwr = ipwr; }
    }

    void setISettings(SDK::Interface::ISettings* isettings) {
        if (isettings) { this->isettings = isettings; }
    }

    void setIFileSystem(SDK::Interface::IFileSystem* ifs) {
        if (ifs) { this->ifs = ifs; }
    }

    void setIBacklight(SDK::Interface::IBacklight* ibacklight) {
        if (ibacklight) { this->ibacklight = ibacklight; }
    }

    void setIVibro(SDK::Interface::IVibro* ivibro) {
        if (ivibro) { this->ivibro = ivibro; }
    }

    void setIBuzzer(SDK::Interface::IBuzzer* ibuzzer) {
        if (ibuzzer) { this->ibuzzer = ibuzzer; }
    }

    void setITime(SDK::Interface::ITime* itime) {
        if (itime) { this->itime = itime; }
    }

    void setISensorManager(SDK::Interface::ISensorManager* isensorManager) {
        if (isensorManager) { this->isensorManager = isensorManager; }
    }

protected:

    bool mIsServise = true;
    Mock::App* mSrvApp = nullptr;
    Sensors::ISensorCore* mSensoreCore = nullptr;
    OS::Mutex mAppMutex;

private:

    Mock::System                        mISystem;
    SDK::Interface::ISystem*            isystem;

    Mock::Logger                        mILogger;
    SDK::Interface::ILogger*            ilogger;

    Mock::AppMemory                     mIAppMemory;
    SDK::Interface::IAppMemory*         imem;

    Mock::App                           mIApp;
    SDK::Simulator::Mock::App*          iappmock;

    Mock::AppCapabilities               mIAppCapabilities;
    SDK::Interface::IAppCapabilities*   iappCapabilities;

    OS::SynchManager                    mSynchManager;
    SDK::Interface::ISynchManager*      isynchManager;

    Mock::ServiceControl&               mServiceControl;
    SDK::Interface::IServiceControl*    isctrl;
    SDK::Interface::IGUIControl*        igctrl;

    Mock::Power                         mIPower;
    SDK::Interface::IPower*             ipwr;

    Mock::Settings                      mISettings;
    SDK::Interface::ISettings*          isettings;

    Mock::FileSystem                    mIFilesystem;
    SDK::Interface::IFileSystem*        ifs;

    Mock::Backlight                     mIBacklight;
    SDK::Interface::IBacklight*         ibacklight;

    Mock::Vibro                         mIVibro;
    SDK::Interface::IVibro*             ivibro;

    Mock::Buzzer                        mIBuzer;
    SDK::Interface::IBuzzer*            ibuzzer;

    Mock::Time                          mITime;
    SDK::Interface::ITime*              itime;

    Mock::SensorManager                 mSensorManager;
    SDK::Interface::ISensorManager*     isensorManager;


    class KIP : public SDK::Interface::IKIP
    {
        friend class KernelBase;
    public:

        KernelBase& base;

        virtual void* queryInterface(SDK::Interface::IKIP::IntfID iid) const override
        {

            switch (iid) {

            case SDK::Interface::IKIP::IntfID::IID_SYSTEM:              return base.isystem;
            case SDK::Interface::IKIP::IntfID::IID_LOGGER:              return base.ilogger;

            case SDK::Interface::IKIP::IntfID::IID_APP_MEMORY:          return base.imem;
            case SDK::Interface::IKIP::IntfID::IID_APP:                 return reinterpret_cast<SDK::Interface::IApp*>(base.iappmock);
            case SDK::Interface::IKIP::IntfID::IID_APP_CAPABILITIES:    return base.iappCapabilities;

            case SDK::Interface::IKIP::IntfID::IID_SYNCH_MANAGER:       return base.isynchManager;
            case SDK::Interface::IKIP::IntfID::IID_SERVICE_CONTROL:     return base.isctrl;
            case SDK::Interface::IKIP::IntfID::IID_GUI_CONTROL:         return base.igctrl;

            case SDK::Interface::IKIP::IntfID::IID_POWER:               return base.ipwr;
            case SDK::Interface::IKIP::IntfID::IID_SETTINGS:            return base.isettings;
            case SDK::Interface::IKIP::IntfID::IID_FILESYSTEM:          return base.ifs;
            case SDK::Interface::IKIP::IntfID::IID_BACKLIGHT:           return base.ibacklight;
            case SDK::Interface::IKIP::IntfID::IID_VIBRO:               return base.ivibro;
            case SDK::Interface::IKIP::IntfID::IID_BUZZER:              return base.ibuzzer;
            case SDK::Interface::IKIP::IntfID::IID_TIME:                return base.itime;

            case SDK::Interface::IKIP::IntfID::IID_SENSOR_MANAGER:      return base.isensorManager;

            default:
                assert(false);
                return nullptr;
            }
        }

        KIP(KernelBase& base) : base(base){}
        virtual ~KIP() = default;
    };
    KIP mKip;
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

} // namespace SDK::Simulator
