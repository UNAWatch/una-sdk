#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include "SDK/Kernel/KernelProviderService.hpp"

class Service
{
public:
    Service(SDK::Kernel& kernel);

    virtual ~Service() = default;

    void run();

private:
    SDK::Kernel&             mKernel;
    bool                     mGUIStarted;

    void onStartGUI();
    void onStopGUI();

    static uint32_t ParseVersion(const char* str);
};

#endif
