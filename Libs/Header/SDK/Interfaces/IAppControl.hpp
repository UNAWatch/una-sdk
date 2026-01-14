/**
 ******************************************************************************
 * @file    IAppControl.hpp
 * @date    03-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * 
 * @brief   An interface for managing the graphical part of the service.
 *          Allows both immediate launch of the GUI and the ability to leave
 *          a pointer to a shared context with subsequent retrieval of it in
 *           the GUI after launch.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <cstdbool>
#include <memory>

namespace SDK::Interface {

/**
 * @class IServiceControl
 * @brief Interface for controling GUI part od the service
 */
class IServiceControl {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IServiceControl() = default;

    /**
     * @brief Starts the GUI part and pass a pointer to the shared context
     * @return        True if GUI started, false otherwise
     */
    virtual bool runGUI()
    {
        return false;
    }

    /**
     * @brief Sets a pointer to a shared context without starting the GUI
     * @param context A pointer to the context
     */
    virtual void setContext(std::shared_ptr<void> context)
    {
        (void) context;
    }
};

/**
 * @class IGUIControl
 * @brief Interface for controling GUI part od the service
 */
class IGUIControl {
public:

    /**
     * @brief Virtual destructor.
     */
    virtual ~IGUIControl() = default;

    /**
     * @brief Gets a pointer to a shared context
     *        The GUI part gets a context with kernel->ctrl.getContext() call
     *        IModel* model = (IModel*) kernel->ctrl.getContext();
     *
     * @return A pointer to the context
     */
    virtual std::shared_ptr<void> getContext()
    {
        return nullptr;
    }
};

} // namespace SDK::Interface
