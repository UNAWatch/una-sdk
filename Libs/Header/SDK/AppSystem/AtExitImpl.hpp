/**
 ******************************************************************************
 * @file    AtExitImpl.hpp
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

/**
 * @brief Register a termination callback with argument and DSO handle.
 *
 * @param func       Function pointer to call on finalize.
 * @param arg        Argument passed to @p func (object pointer for C++ dtors).
 * @param dso_handle DSO handle (used to finalize one shared object), or nullptr.
 * @return 0 on success, non-zero on failure (table full).
 *
 * @note   Callbacks are invoked in reverse registration order by `__cxa_finalize`.
 * @warning Not thread-safe: wrap with a lock if used concurrently.
 */
int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle);

/**
 * @brief ARM EABI equivalent of `__cxa_atexit`.
 *
 * @param obj         Object pointer passed as argument to the destructor.
 * @param dtor        Destructor function taking a `void*`.
 * @param dso_handle  DSO handle (or nullptr).
 * @return 0 on success, non-zero on failure.
 */
int __aeabi_atexit(void* obj, void (*dtor)(void*), void* dso_handle);

/**
 * @brief C-compatible `atexit` wrapper.
 *
 * Registers a callback with no argument and no DSO scoping.
 *
 * @param f Function to call at exit.
 * @return 0 on success, non-zero on failure.
 */
int atexit(void (*f)(void));

/**
 * @brief Get current number of registered (pending) termination callbacks.
 * @return Count of entries that have not yet been finalized.
 */
size_t atexit_registered_count();

/**
 * @brief Invoke registered callbacks in LIFO order.
 *
 * @param dso If non-null, only entries matching @p dso are invoked.
 *            If null, all registered entries are invoked.
 *
 * @note  Each entry is cleared before invocation to prevent double calls,
 *        and @ref mCount is decremented for each consumed entry.
 * @note  Safe to call multiple times; already-invoked entries are ignored.
 * @warning Not thread-safe: ensure single-threaded finalization or guard with a lock.
 */
void __cxa_finalize(void* dso);

}

#endif // __AT_EXIT_IMPL_HPP
