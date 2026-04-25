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

    /**
     * @brief Called by the framework to dispatch an application-specific message
     *        to the handler on the GUI thread.
     *
     * The return value is forwarded back to the original sender via
     * @c sendResponse() as @c MessageResult::SUCCESS (true) or
     * @c MessageResult::FAIL (false).
     *
     * For one-way state-update messages (e.g. Service → GUI notifications)
     * where the sender does not inspect the result, returning @c true
     * unconditionally is appropriate.
     *
     * @param msg  Non-owning pointer to the received message.
     *             The framework releases the message after this call returns.
     * @return @c true  if the message was handled successfully,
     *         @c false if processing failed.
     */
    virtual bool customMessageHandler(SDK::MessageBase *msg) = 0;

protected:

    /**
     * @brief Destructor.
     */
    virtual ~ICustomMessageHandler() = default;
};

} // namespace SDK::Interface
