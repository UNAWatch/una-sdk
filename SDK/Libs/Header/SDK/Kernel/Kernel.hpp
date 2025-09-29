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

    /**
     * @brief Aggregate of kernel service interfaces (non-owning references).
     *
     * @note This façade does not perform any validation or lifetime management.
     *       The caller must ensure that referenced interfaces outlive this object.
     */
    class Kernel {
    public:
        /**
         * @brief Construct the façade by binding kernel service references.
         *
         * @param pwr           Reference to power control interface.
         * @param settings      Reference to settings interface.
         * @param fs            Reference to filesystem interface.
         * @param mem           Reference to user-app memory allocator.
         * @param synchManager  Reference to synchronization manager.
         * @param sensorManager Reference to sensor manager.
         * @param app           Reference to user-app control interface.
         * @param sctrl         Reference to service control interface.
         * @param gctrl         Reference to GUI control interface.
         * @param backlight     Reference to backlight interface.
         * @param vibro         Reference to vibration interface.
         * @param buzzer        Reference to buzzer interface.
         */
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
