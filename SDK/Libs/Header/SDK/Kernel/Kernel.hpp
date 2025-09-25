/**
 ******************************************************************************
 * @file    Kernel.hpp
 * @date    24-September-2025
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

#ifndef __KERNEL_HPP
#define __KERNEL_HPP

#include <cassert>

#include "SDK/Interfaces/IKernel.hpp"

namespace SDK {

    class Kernel {
    public:
        Kernel(SDK::Interface::IPower&               pwr,
               SDK::Interface::ISettings&            settings,
               SDK::Interface::IFileSystem&          fs,
               SDK::Interface::IUserAppMemAllocator& mem,
               SDK::Interface::ISynchManager&        synchManager,
               SDK::Interface::ISensorManager&       sensorManager,
               SDK::Interface::IUserApp&             app,
               SDK::Interface::IServiceControl&      sctrl,
               SDK::Interface::IGUIControl&          gctrl,
               SDK::Interface::IBacklight&           backlight,
               SDK::Interface::IVibro&               vibro,
               SDK::Interface::IBuzzer&              buzzer)
            : pwr(pwr)
            , settings(settings)
            , fs(fs)
            , mem(mem)
            , synchManager(synchManager)
            , sensorManager(sensorManager)
            , app(app)
            , sctrl(sctrl)
            , gctrl(gctrl)
            , backlight(backlight)
            , vibro(vibro)
            , buzzer(buzzer)
        {}

        ~Kernel() = default;

        SDK::Interface::IPower&               pwr;
        SDK::Interface::ISettings&            settings;
        SDK::Interface::IFileSystem&          fs;
        SDK::Interface::IUserAppMemAllocator& mem;
        SDK::Interface::ISynchManager&        synchManager;
        SDK::Interface::ISensorManager&       sensorManager;
        SDK::Interface::IUserApp&             app;
        SDK::Interface::IServiceControl&      sctrl;
        SDK::Interface::IGUIControl&          gctrl;
        SDK::Interface::IBacklight&           backlight;
        SDK::Interface::IVibro&               vibro;
        SDK::Interface::IBuzzer&              buzzer;
    };

}

#endif /* __KERNEL_HPP */
