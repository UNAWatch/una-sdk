/**
 ******************************************************************************
 * @file    Kernel.hpp
 * @date    05-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   The base class of the kernel simulator.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Simulator/Sensors/ISensorCore.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"
#include "SDK/Interfaces/IApp.hpp"

static constexpr char sFsPath[] = "../../../../../Output/";

namespace SDK::Simulator
{

Kernel::Kernel(const char* name, Sensors::ISensorCore* sensoreCore)
    : mSensoreCore(sensoreCore)
    , mIsServise(mSensoreCore != nullptr)
    , mName(name)
    , mLogger()
    , mAppMemory()
    , mFilesystem(sFsPath)
    , mISystem(nullptr)
    , mILogger(&mLogger)
    , mIAppMemory(&mAppMemory)
	, mIAppComm(nullptr)
    , mIFilesystem(&mFilesystem)
    , mKernel()
	, mKernelIsStopped(false)
{
}

SDK::Kernel& Kernel::getKernel()
{
    if (!mKernel) {
        mKernel = std::make_unique<SDK::Kernel>(*((SDK::Interface::ISystem*)queryInterface(SDK::Interface::IKIP::IntfID::IID_SYSTEM)),
                                                *((SDK::Interface::ILogger*)queryInterface(SDK::Interface::IKIP::IntfID::IID_LOGGER)),
                                                *((SDK::Interface::IAppMemory*)queryInterface(SDK::Interface::IKIP::IntfID::IID_APP_MEMORY)),
                                                *((SDK::Interface::IAppComm*)queryInterface(SDK::Interface::IKIP::IntfID::IID_APP_COMM)),
                                                *((SDK::Interface::IFileSystem*)queryInterface(SDK::Interface::IKIP::IntfID::IID_FILESYSTEM)));
    }

    return *mKernel.get();
}

void Kernel::tick()
{
    if (mSensoreCore) {
        mSensoreCore->tick();
    }
}

bool Kernel::keyFilter(uint8_t key)
{
     if (mKernelIsStopped){
        return false;
     }
     
    return (!mIsServise && //iappmock->getState() == Mock::App::State::RESUMED &&
            (SDK::Interface::IApp::Button::BUTTON_L1 == key ||
             SDK::Interface::IApp::Button::BUTTON_L2 == key ||
             SDK::Interface::IApp::Button::BUTTON_R1 == key ||
             SDK::Interface::IApp::Button::BUTTON_R2 == key ||
             SDK::Interface::IApp::Button::BUTTON_L1R2 == key));
}

std::string Kernel::getFsPath()
{
    return mFilesystem.getRootPath();
}

void Kernel::setISystem(SDK::Interface::ISystem* isystem)
{
    if (isystem) {
        mISystem = isystem;
    }
}

void Kernel::setILogger(SDK::Interface::ILogger* ilogger)
{
    if (ilogger) {
        mILogger = ilogger;
    }
}

void Kernel::setIAppMemory(SDK::Interface::IAppMemory* imem)
{
    if (imem) {
        mIAppMemory = imem;
    }
}

void Kernel::setIAppComm(SDK::Interface::IAppComm* comm)
{
    if (comm) {
        mIAppComm = comm;
    }
}

void Kernel::setIFileSystem(SDK::Interface::IFileSystem* ifs)
{
    if (ifs) {
        mIFilesystem = ifs;
    }
}

void* Kernel::queryInterface(SDK::Interface::IKIP::IntfID iid)
{
    switch (iid) {
    case SDK::Interface::IKIP::IntfID::IID_SYSTEM:     return mISystem;
    case SDK::Interface::IKIP::IntfID::IID_LOGGER:     return mILogger;
    case SDK::Interface::IKIP::IntfID::IID_APP_MEMORY: return mIAppMemory;
    case SDK::Interface::IKIP::IntfID::IID_APP_COMM:   return mIAppComm;   
    case SDK::Interface::IKIP::IntfID::IID_FILESYSTEM: return mIFilesystem;

    default:
        assert(false);
        return nullptr;
    }
}

} // namespace SDK::Simulator