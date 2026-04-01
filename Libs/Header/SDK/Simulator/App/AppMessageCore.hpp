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
#include "SDK/Simulator/App/DualAppComm.hpp"

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

        SDK::App::DualAppComm &getAppComm();

    private:
        ::App::MessageManager             mMessageManager;
        SDK::App::DualAppComm              mAppComm;
    };

} // namespace App

#endif // __APP_COMM_HPP
