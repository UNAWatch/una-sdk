/**
 *******************************************************************************
 * @file    AtExitImpl.cpp
 * @date    24-September-2025
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
 *******************************************************************************
 *
 *******************************************************************************
 */

#include "SDK/AppSystem/AtExitImpl.hpp"

#include <cstdint>
#include <cstddef>
#include <atomic>

////////////////////////////////////////////////////////////////////////////////
//// Defines
////////////////////////////////////////////////////////////////////////////////

/**
 * @def ATEXIT_MAX_FUNCS
 * @brief Maximum number of termination callbacks that can be registered.
 * @details Increase this value if you expect many static objects or a large
 *          number of `atexit` registrations. On overflow, registrations fail
 *          and `mOverflow` counter is incremented.
 */
#ifndef ATEXIT_MAX_FUNCS
#define ATEXIT_MAX_FUNCS 32
#endif

////////////////////////////////////////////////////////////////////////////////
//// Data types
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Single entry in the termination-callback registry.
 */
struct AtexitItem {
    void (*fn)(void*);  ///< Callback (C++ dtor thunk or `atexit` shim).
    void* arg;          ///< Argument passed to the callback (for `__cxa_atexit`).
    void* dso;          ///< DSO handle (for shared objects); `nullptr` for plain `atexit`.
    bool  used;         ///< Slot usage flag.
};

////////////////////////////////////////////////////////////////////////////////
//// Private data
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixed-size registry of termination callbacks.
 */
static AtexitItem mTable[ATEXIT_MAX_FUNCS];

/**
 * @brief Number of currently registered (unused) entries.
 * @note  Updated when entries are added or consumed in `__cxa_finalize`.
 */
static std::atomic<size_t> mCount{0};

/**
 * @brief Number of failed registrations due to table overflow.
 */
static std::atomic<size_t> mOverflow{0};

////////////////////////////////////////////////////////////////////////////////
//// Private and system code
////////////////////////////////////////////////////////////////////////////////

extern "C" {

int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle)
{
    // Find a free slot
    for (size_t i = 0; i < ATEXIT_MAX_FUNCS; ++i) {
        if (!mTable[i].used) {
            mTable[i].fn   = func;
            mTable[i].arg  = arg;
            mTable[i].dso  = dso_handle;
            mTable[i].used = true;
            mCount.fetch_add(1, std::memory_order_release);
            return 0;
        }
    }

    mOverflow.fetch_add(1, std::memory_order_relaxed);
    return -1; // ENOMEM
}

int __aeabi_atexit(void* obj, void (*dtor)(void*), void* dso_handle)
{
    return __cxa_atexit(dtor, obj, dso_handle);
}

int atexit(void (*f)(void))
{
    using Fn = void(*)(void*);
    return __cxa_atexit(reinterpret_cast<Fn>(f), nullptr, nullptr);
}

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
//// Public code
////////////////////////////////////////////////////////////////////////////////

extern "C" {

size_t atexit_registered_count()
{
    return mCount.load(std::memory_order_acquire);
}

void __cxa_finalize(void* dso)
{
    // LIFO: traverse from the end of the table
    for (int i = static_cast<int>(ATEXIT_MAX_FUNCS) - 1; i >= 0; --i) {
        AtexitItem& e = mTable[i];

        if (!e.used) {
            continue;
        }

        if (dso != nullptr && e.dso != dso) {
            continue;
        }

        // Mark as consumed prior to invoking the function
        e.used = false;
        mCount.fetch_sub(1, std::memory_order_release);

        if (e.fn) {
            void (*fn)(void*) = e.fn;
            void* arg = e.arg;

            // Clear for safety
            e.fn  = nullptr;
            e.arg = nullptr;
            e.dso = nullptr;

            fn(arg); // Invoke handler/destructor
        }
    }
}

} // extern "C"