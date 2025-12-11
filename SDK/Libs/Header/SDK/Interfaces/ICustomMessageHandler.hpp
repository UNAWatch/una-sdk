/**
 ******************************************************************************
 * @file    ICustomMessageHandler.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for handling application custom messages.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"

namespace SDK::Interface
{

/**
 * @brief Interface for handling application custom messages.
 */
class ICustomMessageHandler {
public:

    virtual bool customMessageHandler(SDK::MessageBase *msg) = 0;

protected:

    /**
     * @brief Destructor.
     */
    virtual ~ICustomMessageHandler() = default;
};

} // namespace SDK::Interface
