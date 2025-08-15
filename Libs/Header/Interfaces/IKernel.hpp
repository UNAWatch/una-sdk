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

// Kernel interfaces
#include "IUserApp.hpp"
#include "IUserAppControl.hpp"
#include "IUserAppMemAllocator.hpp"
#include "ISettings.hpp"
#include "IFileSystem.hpp"
#include "ITime.hpp"
#include "IPower.hpp"
#include "ISynchManager.hpp"
#include "ISensorManager.hpp"
#include "IBacklight.hpp"
#include "IVibro.hpp"
#include "IBuzzer.hpp"

#define KERNEL_INTERFACE_VERSION    (0)

struct IKernel {
    IKernel(Interface::IPower               &pwr,
            Interface::ITime                &time,
            Interface::ISettings            &settings,
            Interface::IFileSystem          &fs,
            Interface::IUserAppMemAllocator &mem,
            Interface::ISynchManager        &synchManager,
            Interface::ISensorManager       &sensorManager,
            Interface::IUserApp             &app,
            Interface::IServiceControl      &sctrl,
            Interface::IGUIControl          &gctrl,
            Interface::IBacklight           &backlight,
            Interface::IVibro               &vibro,
            Interface::IBuzzer              &buzzer)
        : pwr(pwr)
        , time(time)
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
    Interface::IPower    &pwr;
    Interface::ITime     &time;
    Interface::ISettings &settings;

    // Individual interfaces for each app
    Interface::IFileSystem          &fs;
    Interface::IUserAppMemAllocator &mem;
    Interface::ISynchManager        &synchManager;
    Interface::ISensorManager       &sensorManager;
    Interface::IUserApp             &app;

    // Control
    Interface::IServiceControl &sctrl;
    Interface::IGUIControl     &gctrl;

    // Feedback controller interfaces
    Interface::IBacklight &backlight;
    Interface::IVibro     &vibro;
    Interface::IBuzzer    &buzzer;
};

#define DUMMY_KERNEL_ADDR  (0xA5A5A5A5)

#endif /* __INTERFACE_I_KERNEL_HPP */
