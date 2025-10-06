/**
 ******************************************************************************
 * @file    G2SEvents.hpp
 * @date    DD-MM-YYYY
 * @author  Autor
 * @brief   Defines user event types exchanged between the Service and GUI layers.
 *
 * @details
 * This header describes user-defined event data types that are used for
 * inter-thread communication between the Service and GUI parts of the
 * application.
 *
 * It **must** be included in every user application, even if no custom
 * event types are defined.
 * At least one event type — **std::monostate** — must always be present.
 *
 * The 'std::monostate' event is used as a "dummy" signal, for example,
 * to abort waiting for a queue pop operation or to signal a timeout.
 *
 * @note
 * - The user may freely extend the 'G2SEvent::Data' variant with additional
 *   structures representing their own events.
 * - For each new event type, a corresponding 'handleEvent()' method must
 *   be declared in 'IServiceModelHandler'.
 *
 ******************************************************************************
 * @attention
 * Do not remove or rename this file — it is a required part of the framework.
 * Even an empty application must include it, as it defines base event behavior.
 ******************************************************************************
 */

 /**
  * @defgroup G2SEvents GUI-to-Service Event Types
  * @brief User-defined event data types sent from GUI to Service.
  * @{
  */

#pragma once

#include <stdint.h>
#include <variant>

  // Include user-defined typses
  // #include "UserTypes.hpp"


  /**
   * @namespace G2SEvent
   * @brief Namespace that groups all event data types used for communication
   *        from the GUI layer to the Service.
   */
namespace G2SEvent
{

/**
 * @brief Default "empty" event type.
 *
 * This type is always required. It is typically used to:
 * - Interrupt waiting on a queue.
 * - Represent "no data" or a neutral signal.
 */
using Default = std::monostate;



/**
 * @brief Define all possible event types transmitted from GUI to Service.
 *
 * @note The user may extend this list by adding their own types below.
 *       Each new type must also have a corresponding handler in
 *       'IServiceModelHandler'.
 */
using Data = std::variant<
    Default

    // ---------------------------------------------------------------------
    // USER SECTION: Add your custom event types below
    // Example:
    // , struct MyEvent1 { int id; float value; }
    // , struct MyEvent2  { std::vector<MyCustomType> list; }
    // ---------------------------------------------------------------------

>;
}; // namespace G2SEvent


/**
 * @brief Interface for handling Service model events.
 *
 * Each event type in 'G2SEvent::Data' must have a matching handler here.
 */
class IServiceModelHandler {
public:
    virtual ~IServiceModelHandler() = default;

    /**
     * @brief Default handler for std::monostate events.
     *
     * Usually used to handle "abort" or "no-op" signals.
     * The user may override this method if necessary.
     */
    virtual void handleEvent(const std::monostate& event) {};

    // -------------------------------------------------------------------------
    // USER SECTION: Add handlers for your custom event types here
    // Example:
    // virtual void handleEvent(const G2SEvent::MyEvent1& event) = 0;
    // virtual void handleEvent(const G2SEvent::MyEvent2& event) = 0;
    // -------------------------------------------------------------------------
};

/** @} */ // end of G2SEvents