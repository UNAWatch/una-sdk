/**
 * @file   AppCore.hpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#ifndef __APP_CORE_HPP
#define __APP_CORE_HPP

#include <cstdint>

#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Simulator/App/MessageManager.hpp"
#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Simulator/App/DualAppComm.hpp"
#include "SDK/Simulator/Kernel/Mock/Backlight.hpp"
#include "SDK/Simulator/Kernel/Mock/Buzzer.hpp"
#include "SDK/Simulator/Kernel/Mock/Vibro.hpp"

namespace App
{

class Core {
public:
    /**
     * @brief Construct message manager
     */
    Core(SDK::App::DualAppComm &appComm,
         SDK::Simulator::Kernel& srvKernel,
         SDK::Simulator::Kernel& guiKernel);

    /**
     * @brief Destructor
     */
    ~Core() = default;

    void stopRequest();

	void run();
    void runGuiComm();

private:
    bool isStopRequest();

    SDK::App::DualAppComm&  mAppComm;
    SDK::Simulator::Kernel& mSrvKernel;
    SDK::Simulator::Kernel& mGuiKernel;
	volatile bool           mStopRequested;
    OS::Mutex               mMutex;
};

} // namespace App

#endif // __APP_CORE_HPP
