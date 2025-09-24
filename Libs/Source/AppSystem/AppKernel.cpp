/**
 ******************************************************************************
 * @file    AppKernel.cpp
 * @date    24-September-2022
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Client-side Kernel facade that binds interface references via IKernel::queryInterface.
 * @details This wrapper resolves all required subsystem interfaces from a provided @c IKernel
 *          instance and exposes them as references in a fixed, well-known order. Resolution is
 *          performed once in the constructor; each field then aliases the underlying service.
 *
 * @note    All references are borrowed (non-owning) and remain valid only while the source
 *          @c IKernel and the corresponding services are alive.
 * @note    Construction @b asserts on missing interfaces. Replace @c assert with your error
 *          handling if needed.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/AppSystem/AppKernel.hpp"

/// Global kernel pointer (defined elsewhere in system).
extern const IKernel* kernel;

SDK::Kernel& SDK::Kernel::GetInstance()
{
    static SDK::Kernel mInstance;

    return mInstance;
}

SDK::Kernel::Kernel()
    : pwr(          require<SDK::Interface::IPower>(kernel,               IKernel::IntfID::IID_POWER))
    , settings(     require<SDK::Interface::ISettings>(kernel,            IKernel::IntfID::IID_SETTINGS))
    , fs(           require<SDK::Interface::IFileSystem>(kernel,          IKernel::IntfID::IID_FILESYSTEM))
    , mem(          require<SDK::Interface::IUserAppMemAllocator>(kernel, IKernel::IntfID::IID_MEM_ALLOCATOR))
    , synchManager( require<SDK::Interface::ISynchManager>(kernel,        IKernel::IntfID::IID_SYNCH_MANAGER))
    , sensorManager(require<SDK::Interface::ISensorManager>(kernel,       IKernel::IntfID::IID_SENSOR_MANAGER))
    , app(          require<SDK::Interface::IUserApp>(kernel,             IKernel::IntfID::IID_USER_APP))
    , sctrl(        require<SDK::Interface::IServiceControl>(kernel,      IKernel::IntfID::IID_SERVICE_CONTROL))
    , gctrl(        require<SDK::Interface::IGUIControl>(kernel,          IKernel::IntfID::IID_GUI_CONTROL))
    , backlight(    require<SDK::Interface::IBacklight>(kernel,           IKernel::IntfID::IID_BACKLIGHT))
    , vibro(        require<SDK::Interface::IVibro>(kernel,               IKernel::IntfID::IID_VIBRO))
    , buzzer(       require<SDK::Interface::IBuzzer>(kernel,              IKernel::IntfID::IID_BUZZER))
{}
