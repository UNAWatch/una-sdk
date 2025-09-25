/**
 ******************************************************************************
 * @file    KernelBuilder.hpp
 * @date    24-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Helper to construct an SDK::Kernel facade from an IKernel provider.
 * @details Declares a lightweight builder that resolves all required subsystem
 *          interfaces via @c IKernel::queryInterface and returns an initialized
 *          @c SDK::Kernel facade. The @c require<T>() helper asserts that each
 *          requested interface is available and safely casts the returned pointer.
 *
 * @note    This header only declares the builder; the actual construction logic
 *          (e.g., the @c build() definition and how the @c IKernel instance is
 *          obtained) is expected to be provided in the corresponding source file.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __KERNEL_BUILDER_HPP
#define __KERNEL_BUILDER_HPP

#include <cassert>

#include "SDK/Kernel/Kernel.hpp"

namespace SDK {

    class KernelBuilder {
    public:
        static SDK::Kernel make();

    private:
        KernelBuilder()  = delete;
        ~KernelBuilder() = delete;

        template <class T>
        static T& require(const IKernel* k, IKernel::IntfID id) {
            void* p = k->queryInterface(id);
            assert(p && "Kernel::require: requested interface is not available");
            return *static_cast<T*>(p);
        }
    };

}

#endif /* __KERNEL_BUILDER_HPP */
