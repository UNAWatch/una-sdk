/**
 ******************************************************************************
 * @file    KernelManager.hpp
 * @date    17-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   The class which holds a pointer to the IKernel.
 *          This trick is used to place a pointer to an IKernel somewhere early
 *          in the program and retrieve it in the GUI Model class.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __KERNEL_MANAGER_HPP
#define __KERNEL_MANAGER_HPP

#include "SDK/Interfaces/IKernel.hpp"

/**
 * @class KernelManager
 * 
 * Helper class for storing a pointer to the IKernel object.
 */
class KernelManager
{
public:
    static KernelManager& CreateInstance(const IKernel* kernel)
    {
        static bool initialized;
        
        KernelManager& instance = GetInstance();
        
        if (!initialized) {
            instance.init(kernel);
            initialized = true;
        }

        return instance;
    }

    static KernelManager& GetInstance()
    {
        static KernelManager mInstance;

        return mInstance;
    }

    const IKernel* getKernel()
    {
        return mKernel;
    }

private:
    KernelManager() : mKernel(nullptr)
    {}

    void init(const IKernel* kernel)
    {
        mKernel = kernel;
    }
    
    const IKernel* mKernel;
};


#endif /* __KERNEL_MANAGER_HPP */
