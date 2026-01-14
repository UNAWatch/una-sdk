/**
 ******************************************************************************
 * @file    IGUIModel.hpp
 * @date    29-September-2025
 * @author  Oleksandr Tymoshenko
 * @brief   GUI↔Service model interface (non-templated).
 *
 * @details Defines a minimal interface mediating data flow between a GUI task
 *          and a Service task in two directions:
 *            • GUI→Service (G2S): payload type is @c G2SEvent::Data
 *            • Service→GUI (S2G): payload type is @c S2GEvent::Data
 *
 *          Implementations keep non-owning pointers to the GUI kernel facade
 *          and the GUI handler, both of which are provided via
 *          @ref IGUIModel::setGUIHandler and must outlive any calls to
 *          @ref IGUIModel::process.
 ******************************************************************************
 */

#ifndef __I_GUI_MODEL_HPP__
#define __I_GUI_MODEL_HPP__

#include "SDK/Kernel/Kernel.hpp"
#include "GSModelEvents/S2GEvents.hpp"
#include "GSModelEvents/G2SEvents.hpp"
#include <variant>
#include <string>
#include <stdint.h>

/**
 * @brief Interface for bridging GUI and Service layers.
 *
 * @details Declares the minimal API required by GUI-side code to:
 *          - bind GUI context (kernel + handler),
 *          - poll and dispatch Service→GUI events,
 *          - send GUI→Service events.
 *
 * @note    Thread-safety and dispatch semantics (e.g., one event per call vs.
 *          draining) are implementation-defined.
 */
class IGUIModel {
public:
    /**
     * @brief Default constructor.
     */
    IGUIModel() = default;

    virtual ~IGUIModel() = default;

    /**
     * @brief Bind the GUI kernel facade and the GUI-side handler.
     * @param kernel  Pointer to the GUI kernel facade (non-owning).
     * @param handler Pointer to the GUI handler implementation (non-owning).
     *
     * @pre  @p kernel and (if non-null) @p handler must remain valid for as long
     *       as @ref process may be invoked.
     */
    virtual void setGUIHandler(const SDK::Kernel* kernel, IGUIModelHandler* handler) = 0;

    /**
     * @brief Poll and dispatch pending Service→GUI (S2G) events.
     * @param timeout Optional wait time in milliseconds for event availability.
     *                Exact blocking/draining behavior is defined by the
     *                concrete implementation.
     */
    virtual void process(uint32_t timeout = 0) = 0;

    /**
     * @brief Send a GUI→Service (G2S) message.
     * @param data Payload to be delivered to the Service side.
     * @retval true  The message was queued/sent successfully.
     * @retval false The message could not be queued (e.g., queue full).
     */
    virtual bool post(const G2SEvent::Data& data) = 0;
};

#endif // __I_GUI_MODEL_HPP__
