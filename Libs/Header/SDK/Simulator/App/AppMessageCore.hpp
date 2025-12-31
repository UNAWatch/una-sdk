/**
 * @file   MessageCore.hpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application communication (Service + GUI + Kernel)
 */

#ifndef __APP_MESSAGE_CORE_HPP
#define __APP_MESSAGE_CORE_HPP

#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Simulator/App/MessageManager.hpp"
#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Simulator/App/AppComm.hpp"
#include "SDK/Simulator/Kernel/Mock/Backlight.hpp"
#include "SDK/Simulator/Kernel/Mock/Buzzer.hpp"
#include "SDK/Simulator/Kernel/Mock/Vibro.hpp"

namespace SDK::App
{

    class MessageCore
    {
    public:
        /**
         * @brief Construct message manager
         */
        MessageCore();

        /**
         * @brief Destructor
         */
        ~MessageCore() = default;

        SDK::App::Comm& getAppComm();

    private:
        ::App::MessageManager             mMessageManager;
        SDK::Simulator::Mock::Backlight   mBacklight;
        SDK::Simulator::Mock::Buzzer      mBuzzer;
        SDK::Simulator::Mock::Vibro       mVibro;
        SDK::App::KernelMessageDispatcher mKernelMsgDispatcher;
        SDK::App::Comm                    mAppComm;
    };

} // namespace App

#endif // __APP_COMM_HPP
