/**
 ******************************************************************************
 * @file    IGUIModel.hpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Templated GUI model interface bridging GUI and Service layers.
 * @details Defines a lightweight, header-only interface that mediates data flow
 *          between a GUI task and a Service task. The direction of messages is
 *          parameterized:
 *          - @tparam G2S is the GUI→Service payload type handled by
 *            @ref IGUIModel::sendToService.
 *          - @tparam GH  is the GUI-side handler type stored via
 *            @ref IGUIModel::setGUIHandler and used by implementations to
 *            deliver Service→GUI events inside @ref IGUIModel::checkS2GEvents.
 *
 *          The interface keeps non-owning pointers to the GUI kernel facade and
 *          the GUI handler. Ownership/lifetime management is the caller’s
 *          responsibility.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_GUI_MODEL_HPP__
#define __I_GUI_MODEL_HPP__

#include "SDK/Kernel/Kernel.hpp"
#include <variant>
#include <string>
#include <stdint.h>

/**
 * @tparam G2S  Type of GUI→Service message payload sent by @ref sendToService.
 * @tparam GH   Type of GUI-side handler that receives Service→GUI events.
 */
template<typename G2S, typename GH>
class IGUIModel {
public:
    /**
     * @brief Default constructor.
     * @post  @ref mGUIHandler and @ref mGUIKernel are set to @c nullptr.
     */
    IGUIModel();

    /**
     * @brief Virtual destructor.
     */
    virtual ~IGUIModel();

    /**
     * @brief Bind the GUI kernel facade and the GUI-side handler.
     * @param kernel  Pointer to the GUI kernel facade (non-owning).
     * @param handler Pointer to the GUI handler implementation (non-owning).
     * @note  Pointers are stored as-is; the caller must guarantee their lifetime.
     */
    void setGUIHandler(const SDK::Kernel* kernel, GH* handler);

    /**
     * @brief Process pending Service→GUI (S2G) events.
     * @param timeout Optional wait time in milliseconds for event availability.
     *                The exact semantics (blocking/non-blocking) are defined by
     *                the concrete implementation.
     */
    virtual void checkS2GEvents(uint32_t timeout = 0) = 0;

    /**
     * @brief Send a GUI→Service (G2S) message.
     * @param data  Payload to be sent to the Service side.
     * @return @c true on successful queueing/sending, @c false otherwise.
     */
    virtual bool sendToService(const G2S& data) = 0;

protected:
    /**
     * @brief Pointer to the GUI-side handler (non-owning).
     * @note  May be @c nullptr until @ref setGUIHandler is called.
     */
    GH* mGUIHandler;

    /**
     * @brief Pointer to the GUI kernel facade (non-owning).
     * @note  May be @c nullptr until @ref setGUIHandler is called.
     */
    const SDK::Kernel* mGUIKernel;
};

// ---------- Implementation (must stay in header for templates) ----------

template<typename G2S, typename GH>
IGUIModel<G2S, GH>::IGUIModel()
    : mGUIHandler(nullptr)
    , mGUIKernel(nullptr)
{}

template<typename G2S, typename GH>
IGUIModel<G2S, GH>::~IGUIModel() = default;

template<typename G2S, typename GH>
void IGUIModel<G2S, GH>::setGUIHandler(const SDK::Kernel* kernel, GH* handler)
{
    mGUIKernel  = kernel;
    mGUIHandler = handler;
}

#endif // __IMODEL_HPP__


