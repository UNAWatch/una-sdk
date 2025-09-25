/**
 ******************************************************************************
 * @file    KernelProviderService.hpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   This class holds a pointer to the SDK::Kernel object.
 *          This trick is used to place a pointer to an SDK::Kernel somewhere early
 *          in the program and retrieve it in the Service application.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __KERNEL_PROVIDER_SERVICE_HPP
#define __KERNEL_PROVIDER_SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"

namespace SDK {

    /**
     * @class KernelProviderService
     *
     * Helper class for storing a pointer to the SDK::Kernel object.
     */
    class KernelProviderService
    {
    public:
        static KernelProviderService& CreateInstance(const SDK::Kernel* kernel)
        {
            static bool initialized;

            KernelProviderService& instance = GetInstance();

            if (!initialized) {
                instance.init(kernel);
                initialized = true;
            }

            return instance;
        }

        static KernelProviderService& GetInstance()
        {
            static KernelProviderService mInstance;

            return mInstance;
        }

        const SDK::Kernel& getKernel()
        {
            return *mKernel;
        }

    private:
        KernelProviderService() : mKernel(nullptr)
        {}

        void init(const SDK::Kernel* kernel)
        {
            mKernel = kernel;
        }
        
        const SDK::Kernel* mKernel;
    };

}

#endif /* __KERNEL_PROVIDER_SERVICE_HPP */
