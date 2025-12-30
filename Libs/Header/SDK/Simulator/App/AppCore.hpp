/**
 * @file   AppCore.hpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#ifndef __APP_CORE_HPP
#define __APP_CORE_HPP

#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Simulator/App/MessageManager.hpp"
#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Simulator/App/AppComm.hpp"
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
    Core();

    /**
     * @brief Destructor
     */
    ~Core() = default;

	void run();

private:
    ::App::MessageManager             mMessageManager;
	SDK::Simulator::Mock::Backlight   mBacklight;
	SDK::Simulator::Mock::Buzzer      mBuzzer;
	SDK::Simulator::Mock::Vibro       mVibro;
	SDK::App::KernelMessageDispatcher mKernelMsgDispatcher;
    SDK::App::Comm                    mAppComm;
};

} // namespace App

#endif // __APP_CORE_HPP
