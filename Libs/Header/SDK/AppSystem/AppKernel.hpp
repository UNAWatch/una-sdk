/**
 ******************************************************************************
 * @file    AppKernel.hpp
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

#ifndef __APP_KERNEL_HPP
#define __APP_KERNEL_HPP

#include <cassert>

#include "SDK/Interfaces/IKernel.hpp"

namespace SDK {

    class Kernel {
    public:
        static Kernel& GetInstance();

        SDK::Interface::IPower               &pwr;
        SDK::Interface::ISettings            &settings;
        SDK::Interface::IFileSystem          &fs;
        SDK::Interface::IUserAppMemAllocator &mem;
        SDK::Interface::ISynchManager        &synchManager;
        SDK::Interface::ISensorManager       &sensorManager;
        SDK::Interface::IUserApp             &app;
        SDK::Interface::IServiceControl      &sctrl;
        SDK::Interface::IGUIControl          &gctrl;
        SDK::Interface::IBacklight           &backlight;
        SDK::Interface::IVibro               &vibro;
        SDK::Interface::IBuzzer              &buzzer;

    private:
        Kernel();

        template <class T>
        static T& require(const IKernel* k, IKernel::IntfID id) {
            void* p = k->queryInterface(id);
            assert(p && "Kernel::require: requested interface is not available");
            return *static_cast<T*>(p);
        }
    };

}

#endif /* __APP_KERNEL_HPP */
