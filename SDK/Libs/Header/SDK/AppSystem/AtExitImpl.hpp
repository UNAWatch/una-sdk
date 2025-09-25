/**
 ******************************************************************************
 * @file    AtExitImpl.рpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Minimal atexit / __cxa_atexit / __cxa_finalize runtime for bare-metal.
 * @details Provides a small, malloc-free registry for C/C++ termination callbacks:
 *          - `__cxa_atexit(func, arg, dso)` to register C++ static object destructors,
 *          - `atexit(func)` compatibility wrapper,
 *          - `__cxa_finalize(dso)` to invoke callbacks in LIFO order.
 *
 *          The implementation uses a fixed-size table (no heap) and simple
 *          atomic counters. It is suitable for single-threaded or early-boot
 *          environments. If used in multi-threaded systems, protect registration
 *          and finalization with appropriate synchronization.
 *
 * @note    This module does not allocate memory and is safe for freestanding targets.
 * @note    Call `__cxa_finalize(nullptr)` during process/app exit to run all handlers.
 * @note    `__aeabi_atexit` is also provided to satisfy ARM EABI toolchains.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __AT_EXIT_IMPL_HPP
#define __AT_EXIT_IMPL_HPP

#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cerrno>
#include <cassert>

extern "C" {

size_t atexit_registered_count();
void   __cxa_finalize(void* dso);

}

#endif // __AT_EXIT_IMPL_HPP
