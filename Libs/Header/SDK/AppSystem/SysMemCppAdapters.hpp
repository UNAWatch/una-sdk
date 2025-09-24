/**
 * @file    UserAppCppAdapters.hpp
 * @brief   C++ adapters for C runtime hooks that use kernel services.
 * @details Provides inline C++ wrappers that call into the client-side Kernel
 *          facade instead of using a global `kernel` pointer. Include this
 *          header and forward each C hook to its `_cpp_adapter` twin.
 *
 * @note    You must provide an accessor `SDK::Kernel::GetInstance()` that
 *          returns a reference to your client-side Kernel facade which exposes
 *          `.app`, `.mem`, etc.
 *
 * @code
 * extern "C" void* _sbrk(ptrdiff_t incr) { return _sbrk_cpp_adapter(incr); }
 * @endcode
 */

#ifndef USER_APP_CPP_ADAPTERS_HPP
#define USER_APP_CPP_ADAPTERS_HPP

#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cerrno>
#include <cassert>

#include "SDK/AppSystem/AppKernel.hpp"

struct _reent; // forward decl (newlib)

/**
 * @brief C++ adapter for _sbrk.
 */
inline void* _sbrk_cpp_adapter(ptrdiff_t incr) noexcept
{
    SDK::Kernel::GetInstance().app.log("_sbrk %d\n", (int)incr);
    assert(0 && "_sbrk must not be used in user app");
    errno = ENOMEM;
    return (void*)-1;
}

/**
 * @brief C++ adapter for reentrant malloc.
 */
inline void* _malloc_r_cpp_adapter(_reent* r, size_t size) noexcept
{
    (void)r;
    void* ptr = SDK::Kernel::GetInstance().mem.malloc(size);
    SDK::Kernel::GetInstance().app.log("malloc 0x%08" PRIx32 " %u b\n",
                                       (uint32_t)(uintptr_t)ptr, (unsigned)size);
    return ptr;
}

/**
 * @brief C++ adapter for reentrant free.
 */
inline void _free_r_cpp_adapter(_reent* r, void* ptr) noexcept
{
    (void)r;

    if (ptr) {
        SDK::Kernel::GetInstance().app.log("free   0x%08" PRIx32 "\n", (uint32_t)(uintptr_t)ptr);
        SDK::Kernel::GetInstance().mem.free(ptr);
    }
}

/**
 * @brief C++ adapter for reentrant realloc.
 */
inline void* _realloc_r_cpp_adapter(_reent* r, void* ptr, size_t new_size) noexcept
{
    (void)r;
    return SDK::Kernel::GetInstance().mem.realloc(ptr, new_size);
}

/**
 * @brief C++ adapter for assert handler.
 * @note  Must be marked noreturn by the C wrapper to match ABI.
 */
inline void __assert_func_cpp_adapter(const char* file,
                                      int         line,
                                      const char* func,
                                      const char* failedexpr) noexcept
{
    SDK::Kernel::GetInstance().app.log("assert: %s %d %s %s\n", file, line, func, failedexpr);
}

/**
 * @brief C++ adapter for exitA.
 * @note  Must be marked noreturn by the C wrapper to match ABI.
 */
inline void exitA_cpp_adapter(int status) noexcept
{
    SDK::Kernel::GetInstance().app.log("exit %d\n", status);
    SDK::Kernel::GetInstance().app.exit(status);
}

#endif // USER_APP_CPP_ADAPTERS_HPP
