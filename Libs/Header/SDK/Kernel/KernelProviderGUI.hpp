/**
 ******************************************************************************
 * @file    KernelProviderGUI.hpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   This class holds a pointer to the SDK::Kernel object.
 *          This trick is used to place a pointer to an SDK::Kernel somewhere early
 *          in the program and retrieve it in the GUI application.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Kernel/Kernel.hpp"

namespace SDK {

/**
 * @class KernelProviderGUI
 *
 * Helper class for storing a pointer to the SDK::Kernel object.
 */
class KernelProviderGUI
{
public:
    static KernelProviderGUI& CreateInstance(const SDK::Kernel* kernel)
    {
        static bool initialized;

        KernelProviderGUI& instance = GetInstance();

        if (!initialized) {
            instance.init(kernel);
            initialized = true;
        }

        return instance;
    }

    static KernelProviderGUI& GetInstance()
    {
        static KernelProviderGUI mInstance;

        return mInstance;
    }

    const SDK::Kernel& getKernel()
    {
        return *mKernel;
    }

private:
    KernelProviderGUI() : mKernel(nullptr)
    {}

    void init(const SDK::Kernel* kernel)
    {
        mKernel = kernel;
    }
    
    const SDK::Kernel* mKernel;
};

} // namespace SDK
