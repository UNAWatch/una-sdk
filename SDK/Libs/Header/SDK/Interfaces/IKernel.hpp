/**
 ******************************************************************************
 * @file    IKernel.hpp
 * @date    05-09-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   This struct contains APIs for calling system functions
            It is basically a jump table.
            Depending on weather the KERNEL is built or the libs for the app,
            different sys_struct definitions are compiled.
            For Kernel:
                The complete struct with each member is compiled
            For userlib for the app:
                Only a 'const sys_struct* sys' pointer is defined. The address
                of this pointer is made to point to the actual sys_struct in
                the kernel at load time in the LoadApp API.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_KERNEL_HPP
#define __INTERFACE_I_KERNEL_HPP

#include <cstdint>
#include <cstddef>

#include "IKernel.hpp"

// Kernel interfaces
#include "SDK/Interfaces/IPower.hpp"
#include "SDK/Interfaces/ISettings.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Interfaces/IUserAppMemAllocator.hpp"
#include "SDK/Interfaces/ISynchManager.hpp"
#include "SDK/Interfaces/ISensorManager.hpp"
#include "SDK/Interfaces/IUserApp.hpp"
#include "SDK/Interfaces/IUserAppControl.hpp"
#include "SDK/Interfaces/IBacklight.hpp"
#include "SDK/Interfaces/IVibro.hpp"
#include "SDK/Interfaces/IBuzzer.hpp"

#define KERNEL_INTERFACE_VERSION    (0)

struct IKernel {
    IKernel(SDK::Interface::IPower               &pwr,
            SDK::Interface::ISettings            &settings,
            SDK::Interface::IFileSystem          &fs,
            SDK::Interface::IUserAppMemAllocator &mem,
            SDK::Interface::ISynchManager        &synchManager,
            SDK::Interface::ISensorManager       &sensorManager,
            SDK::Interface::IUserApp             &app,
            SDK::Interface::IServiceControl      &sctrl,
            SDK::Interface::IGUIControl          &gctrl,
            SDK::Interface::IBacklight           &backlight,
            SDK::Interface::IVibro               &vibro,
            SDK::Interface::IBuzzer              &buzzer)
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
    {
    }

    const uint32_t version = KERNEL_INTERFACE_VERSION;

    // Common interfaces
    SDK::Interface::IPower    &pwr;
    SDK::Interface::ISettings &settings;

    // Individual interfaces for each app
    SDK::Interface::IFileSystem          &fs;
    SDK::Interface::IUserAppMemAllocator &mem;
    SDK::Interface::ISynchManager        &synchManager;
    SDK::Interface::ISensorManager       &sensorManager;
    SDK::Interface::IUserApp             &app;

    // Control
    SDK::Interface::IServiceControl &sctrl;
    SDK::Interface::IGUIControl     &gctrl;

    // Feedback controller interfaces
    SDK::Interface::IBacklight &backlight;
    SDK::Interface::IVibro     &vibro;
    SDK::Interface::IBuzzer    &buzzer;
};

#define DUMMY_KERNEL_ADDR  (0xA5A5A5A5)

#endif /* __INTERFACE_I_KERNEL_HPP */
