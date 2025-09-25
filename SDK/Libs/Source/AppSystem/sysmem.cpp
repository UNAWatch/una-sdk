/**
 ******************************************************************************
 * @file    sysmem.cpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   System runtime glue for user apps (memory hooks, init/fini, exit).
 * @details This module wires C/C++ runtime entry points to the platform kernel:
 *          - Redirects allocation hooks (`_malloc_r/_free_r/_realloc_r`, `new/delete`)
 *            to the SDK memory interface via C++ adapters declared in
 *            `SDK/AppSystem/SysMemCppAdapters.hpp`.
 *          - Provides `_sbrk` stub for freestanding targets (must not be used).
 *          - Integrates minimal `__cxa_atexit/atexit/__cxa_finalize` (from
 *            `AtExitImpl.hpp`) to run C++ static destructors and atexit handlers on `exit()`.
 *          - Invokes constructor/destructor arrays (`__una_init_array`, `__una_fini_array`).
 *          - Bridges assert/abort/exit to kernel logging and app termination.
 *
 *          A global kernel pointer is placed in a dedicated section (".sys_calls")
 *          and validated at startup (`una_check_kernel`). The loader patches the
 *          placeholder address (`DUMMY_KERNEL_ADDR`) with the actual kernel address.
 *
 * @note    All returned interface pointers from the kernel are borrowed (non-owning).
 * @note    This file targets freestanding/bare-metal environments; behavior differs
 *          from hosted libc implementations.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include <errno.h>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <inttypes.h>

#include "SDK/AppSystem/AtExitImpl.hpp"
#include "SDK/Interfaces/IKernel.hpp"

/**
 * @brief  Global kernel pointer placed into a dedicated section for patching.
 * @details The value is initialized with @c DUMMY_KERNEL_ADDR and must be patched
 *          by the loader to the actual kernel address prior to use.
 */
const IKernel *kernel __attribute__((unused, section(".sys_calls"))) = (IKernel*) (DUMMY_KERNEL_ADDR);

/** @name C++ init/fini arrays (toolchain-provided)
 *  @brief Weak symbols marking constructor/destructor arrays.
 *  @{ */
extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end [])   (void) __attribute__((weak));
extern void (*__init_array_start [])    (void) __attribute__((weak));
extern void (*__init_array_end [])      (void) __attribute__((weak));
extern void (*__fini_array_start[])     (void) __attribute__((weak));
extern void (*__fini_array_end[])       (void) __attribute__((weak));
/** @} */

extern "C" {

    /**
     * @brief  Sanity-check kernel pointer and interface version.
     * @details Verifies that @ref kernel is patched (not equal to @c DUMMY_KERNEL_ADDR)
     *          and that the interface version matches @c KERNEL_INTERFACE_VERSION.
     *          If either check fails, the function loops forever or terminates.
     */
    void una_check_kernel() {
        if ((uintptr_t)kernel == (uintptr_t)DUMMY_KERNEL_ADDR) {
            for (;;) { /* stop */ }
        }

        if (kernel->version != KERNEL_INTERFACE_VERSION) {
            exit(-4);
        }
    }

    /**
     * @brief  Call all registered preinit/init constructors.
     * @details Executes functions from @ref __preinit_array_start .. @ref __preinit_array_end
     *          and then from @ref __init_array_start .. @ref __init_array_end.
     *          This should be called once during system startup.
     */
    void __una_init_array (void)
    {
        size_t count;
        size_t i;

        count = __preinit_array_end - __preinit_array_start;
        for (i = 0; i < count; ++i) {
            __preinit_array_start[i] ();
        }

        #ifdef _HAVE_INIT_FINI
          _init ();
        #endif

        count = __init_array_end - __init_array_start;
        for (i = 0; i < count; ++i) {
           __init_array_start[i] ();
        }
   }

    /**
     * @brief  Call all registered finalizers (C-level).
     * @details Executes functions from @ref __fini_array_start .. @ref __fini_array_end.
     *          Usually invoked during program termination (@ref exit()).
     */
    void __una_fini_array(void)
    {
        int count = __fini_array_end - __fini_array_start;
        for (int i = 0; i < count; ++i) {
            __fini_array_start[i] ();
        }
    }
}

/**
 * @brief   _sbrk() allocates memory to the newlib heap and is used by malloc and others.
 * @details Dynamic memory allocation occurs through the system kernel interface in this
 *          environment, so this function is redirected to the C++ adapter, which logs
 *          and returns failure.
 *
 * @param   incr Requested heap increment in bytes.
 * @return  Always returns @c (void*)-1 and sets @c errno=ENOMEM.
 */

extern "C" {

    /**
     * @brief  C++ ABI handle for DSO finalization bookkeeping (newlib/gcc).
     * @note   Kept as a weak, minimal definition for freestanding targets.
     */
    void* __dso_handle = nullptr;

    /**
     * @brief   System heap extension hook (must not be used).
     * @param   incr Requested increment in bytes.
     * @return  Always @c (void*)-1. See @ref _sbrk_cpp_adapter.
     */
    void* _sbrk(ptrdiff_t incr)
    {
        kernel->app.log("_sbrk %d\n", (int)incr);
        assert(0 && "_sbrk must not be used in user app");
        errno = ENOMEM;
        return (void*)-1;
    }

    /**
     * @brief   malloc wrapper that forwards to reentrant version.
     * @param   size Number of bytes to allocate.
     * @return  Pointer to allocated block or @c nullptr on failure.
     */
    void* malloc(size_t size)
    {
        return _malloc_r(NULL, size);
    }

    /**
     * @brief   free wrapper that forwards to reentrant version.
     * @param   ptr Pointer previously allocated by @ref malloc / @ref _malloc_r.
     */
    void free(void* ptr)
    {
        _free_r(NULL, ptr);
    }

    /**
     * @brief   calloc wrapper using @ref malloc and zeroing.
     * @param   __nmemb Number of elements.
     * @param   __size  Size of each element.
     * @return  Pointer to zeroed block or @c nullptr on failure.
     */
    void* calloc(size_t __nmemb, size_t __size)
    {
        size_t bytes = __nmemb * __size;

        void* res = malloc(bytes);

        if (res) {
            memset(res, 0, bytes);
        }

        return res;
    }

    /**
     * @brief   realloc wrapper that forwards to reentrant version.
     * @param   ptr     Existing block (or @c nullptr).
     * @param   __size  New size in bytes.
     * @return  Pointer to reallocated block or @c nullptr on failure.
     */
    void* realloc(void* ptr, size_t __size)
    {
        return _realloc_r(NULL, ptr, __size);
    }

    /**
     * @brief   Reentrant malloc forwarding to kernel allocator via C++ adapter.
     * @param   r    Reentrancy context (unused).
     * @param   size Number of bytes to allocate.
     * @return  Pointer to allocated block or @c nullptr on failure.
     */
    void* _malloc_r(struct _reent *r, size_t size)
    {
        (void)r;
        void* ptr = kernel->mem.malloc(size);
        kernel->app.log("malloc 0x%08" PRIx32 " %u b\n", (uint32_t)(uintptr_t)ptr, (unsigned)size);
        return ptr;
    }

    /**
     * @brief   Reentrant free forwarding to kernel allocator via C++ adapter.
     * @param   r   Reentrancy context (unused).
     * @param   ptr Pointer to free (nullable).
     */
    void _free_r(struct _reent *r, void *ptr)
    {
        (void)r;

        if (ptr) {
            kernel->app.log("free   0x%08" PRIx32 "\n", (uint32_t)(uintptr_t)ptr);
            kernel->mem.free(ptr);
        }
    }

    /**
     * @brief   Reentrant calloc implemented via @_malloc_r and memset.
     * @param   r     Reentrancy context (unused).
     * @param   nmemb Number of elements.
     * @param   size  Size of each element.
     * @return  Zeroed memory block or @c nullptr on failure.
     */
    void* _calloc_r(struct _reent *r, size_t nmemb, size_t size)
    {
        (void) r;
        void *ptr = _malloc_r(r, nmemb * size);
        if (ptr) {
            memset(ptr, 0, nmemb * size);
        }
        return ptr;
    }

    /**
     * @brief   Reentrant realloc forwarding to kernel allocator via C++ adapter.
     * @param   r        Reentrancy context (unused).
     * @param   ptr      Existing block (or @c nullptr).
     * @param   new_size New size in bytes.
     * @return  Reallocated block or @c nullptr on failure.
     */
    void* _realloc_r(struct _reent *r, void *ptr, size_t new_size)
    {
        (void)r;
        return kernel->mem.realloc(ptr, new_size);
    }

    /**
     * @brief   Called when a pure virtual function is invoked (ODR violation).
     * @warning Marked @c noreturn; implementation traps via @ref assert(false).
     */
    __attribute__((noreturn)) void __cxa_pure_virtual()
    {
        assert(false);
    }

    /**
     * @brief   Guard acquisition for function-local statics (Itanium ABI).
     * @param   g Guard object storage.
     * @return  Non-zero if initialization is required; zero otherwise.
     */
    int __cxa_guard_acquire(uint64_t* g)
    {
        return !*(char*)(g);
    }

    /**
     * @brief   Guard release for function-local statics.
     * @param   g Guard object storage.
     */
    void __cxa_guard_release(uint64_t* g)
    {
        *(char*)(g) = 1;
    }

    /**
     * @brief   Guard abort for function-local statics.
     * @param   (unused)
     */
    void __cxa_guard_abort(uint64_t*)
    {
    }

    /**
     * @brief   Assertion failure handler.
     * @details Logs details via C++ adapter and terminates with @c exit(-1).
     *
     * @param   file       Source file name.
     * @param   line       Source line number.
     * @param   func       Function name.
     * @param   failedexpr Failed expression string.
     * @note    Marked @c noreturn.
     */
    __attribute__((noreturn)) void __assert_func(const char *file,
                                                 int         line,
                                                 const char *func,
                                                 const char *failedexpr)
    {
        kernel->app.log("assert: %s %d %s %s\n", file, line, func, failedexpr);
        exit(-1);
    }

    /**
     * @brief   Abnormal termination; forwards to @ref exit(-1).
     * @note    Marked @c noreturn.
     */
    __attribute__((noreturn)) void abort(void)
    {
        exit(-1);
    }

    /**
     * @brief   Platform-specific final termination path.
     * @details Logs and notifies kernel via C++ adapter, then halts.
     * @param   status Exit status code.
     * @note    Marked @c noreturn.
     */
    __attribute__((noreturn)) void exitA(int status)
    {
        kernel->app.log("exit %d\n", status);
        kernel->app.exit(status);

        for (;;) { /* stop */ }
    }

    /**
     * @brief   Program termination sequence.
     * @details Runs all @c __cxa_atexit/ @c atexit handlers (LIFO), then calls
     *          C-level finalizers via @ref __una_fini_array, and finally transfers
     *          control to @ref exitA.
     * @param   status Exit status code.
     * @note    Marked @c noreturn.
     */
    __attribute__((noreturn)) void exit(int status)
    {
        // 1) Invoke all registered C++/C termination handlers (LIFO)
        __cxa_finalize(nullptr);

        // 2) Invoke C finalizers from .fini_array
        __una_fini_array();

        // 3) Platform-specific final exit (never returns)
        exitA(status);

        for (;;) { /* stop */ }
    }
}

namespace __gnu_cxx {
    /**
     * @brief Weak fallback for verbose terminate handler (libstdc++).
     * @note  Traps forever; override if needed.
     */
    __attribute__((weak)) void __verbose_terminate_handler()
    {
        for (;;) { /* stop */ }
    }
}

namespace std
{
    /**
     * @brief   Throw helper for std::bad_alloc in freestanding mode.
     * @note    Weak symbol; current implementation traps.
     */
    __attribute__((weak)) void __throw_bad_alloc()
    {
        assert(false);
    }

    /**
     * @brief   Throw helper for bad array new length in freestanding mode.
     * @note    Weak symbol; current implementation traps.
     */
    __attribute__((weak)) void __throw_bad_array_new_length()
    {
        assert(false);
    }

    /**
     * @brief   Throw helper for std::length_error in freestanding mode.
     * @param   msg Diagnostic message (unused).
     * @note    Weak symbol; current implementation traps.
     */
    __attribute__((weak)) void __throw_length_error(const char* msg)
    {
        (void) msg;

        assert(false);
    }
}

/**
 * @brief   Global operator new using reentrant malloc.
 * @param   size Number of bytes to allocate.
 * @return  Pointer to allocated memory; @c nullptr on failure.
 * @note    Marked @c noexcept to match freestanding constraints.
 */
void* operator new(std::size_t size) noexcept
{
    return _malloc_r(nullptr, size);
}

/**
 * @brief   Global operator delete using reentrant free.
 * @param   ptr Pointer to memory to be freed (nullable).
 * @note    Marked @c noexcept to match freestanding constraints.
 */
void operator delete(void* ptr) noexcept
{
    _free_r(nullptr, ptr);
}

/**
 * @brief   Global array new forwarding to scalar @ref operator new.
 * @param   size Number of bytes to allocate.
 * @return  Pointer to allocated memory; @c nullptr on failure.
 */
void* operator new[](std::size_t size)
{
    return operator new(size);
}

/**
 * @brief   Global array delete forwarding to scalar @ref operator delete.
 * @param   ptr Pointer to memory to be freed (nullable).
 */
void operator delete[](void* ptr) noexcept
{
    operator delete(ptr);
}

/**
 * @brief   Sized delete (scalar) forwarding to unsized @ref operator delete.
 * @param   ptr Pointer to memory to be freed (nullable).
 * @param   (unused) Size parameter ignored.
 * @note    Marked @c noexcept to match freestanding constraints.
 */
void operator delete(void* ptr, std::size_t) noexcept
{
    operator delete(ptr);
}

/**
 * @brief   Sized delete (array) forwarding to unsized @ref operator delete[].
 * @param   ptr Pointer to memory to be freed (nullable).
 * @param   (unused) Size parameter ignored.
 * @note    Marked @c noexcept to match freestanding constraints.
 */
void operator delete[](void* ptr, std::size_t) noexcept
{
    operator delete[](ptr);
}
