#include "SDK/Kernel/KernelBuilder.hpp"

SDK::Kernel SDK::KernelBuilder::make(const SDK::Interface::IKernel* ikernel)
{
    SDK::Kernel k(require<SDK::Interface::ISystem>(ikernel,     SDK::Interface::IKIP::IntfID::IID_SYSTEM),
                  require<SDK::Interface::ILogger>(ikernel,     SDK::Interface::IKIP::IntfID::IID_LOGGER),
                  require<SDK::Interface::IAppMemory>(ikernel,  SDK::Interface::IKIP::IntfID::IID_APP_MEMORY),
                  require<SDK::Interface::IAppComm>(ikernel,    SDK::Interface::IKIP::IntfID::IID_APP_COMM),
                  require<SDK::Interface::IFileSystem>(ikernel, SDK::Interface::IKIP::IntfID::IID_FILESYSTEM));

    return k;
}