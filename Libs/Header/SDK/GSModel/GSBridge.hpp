/**
 ******************************************************************************
 * @file    GSBridge.hpp
 * @date    29-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   GUI<->Service bridge with bidirectional event queues
 *
 * @details A lightweight mediator that relays events between a GUI thread/task
 *          and a Service thread/task using two ring buffers:
 *            • G2S — GUI -> Service events (type: G2SEvent::Data)
 *            • S2G — Service -> GUI events (type: S2GEvent::Data)
 *
 *          The bridge integrates with the SDK kernel facade to briefly release
 *          the current app lock around queue pop operations so the opposite
 *          side can make progress. Events are held as @c std::variant payloads
 *          and dispatched via @c std::visit to strongly‑typed handler methods.
 *
 * @note    This version is non‑templated and uses fixed event variant types.
 *          It is deliberately small and header‑only for easy embedding.
 ******************************************************************************
 */

#ifndef __GS_BRIDGE_HPP
#define __GS_BRIDGE_HPP

#include "SDK/GSModel/IGUIModel.hpp"
#include "SDK/CircularBuffer.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

#include <type_traits>
#include <variant>

/**
 * @brief GUI<->Service bridge that queues and dispatches events in both directions.
 *
 * @details Owns two ring buffers:
 *          - @ref mG2SQueue for GUI->Service traffic (written by GUI, read by Service).
 *          - @ref mS2GQueue for Service->GUI traffic (written by Service, read by GUI).
 *
 *          Dispatching is performed with @c std::visit over @c std::variant
 *          event payloads (see @c G2SEvent::Data and @c S2GEvent::Data).
 *
 * @par Concurrency
 *          Pop operations temporarily release the app lock via the SDK kernel
 *          facade (see calls to @c app.unLock()/@c app.lock()). Adjust to your
 *          RTOS/critical‑section model if necessary.
 *
 * @warning Lifetime of the GUI handler and @ref mGUIKernel must outlive the
 *          period when the bridge is registered/used by the GUI side.
 */
class GSBridge : public IGUIModel
{
public:
    /**
     * @brief Construct the model and initialize queues.
     * @param serviceHandler Reference to the Service-side handler implementation.
     *
     * @post Queues are initialized; the Service kernel facade is cached.
     */
    GSBridge(IServiceModelHandler& serviceHandler)
        : mServiceKernel(SDK::KernelProviderService::GetInstance().getKernel())
        , mGUIKernel(nullptr)
        , mS2GQueue(mServiceKernel)
        , mG2SQueue(mServiceKernel)
        , mServiceHandler(serviceHandler)
        , mGUIHandler(nullptr)
    {
        mS2GQueue.init();
        mG2SQueue.init();
    }

    /**
     * @brief Virtual destructor (no-op).
     */
    ~GSBridge() override = default;

    /**
     * @brief Drain GUI->Service queue and dispatch to the Service handler.
     *
     * Attempts to pop one G2S message from @ref mG2SQueue with an optional
     * timeout and, if successful, dispatches it to @ref mServiceHandler via
     * @c std::visit.
     *
     * @param timeout Pop wait time in milliseconds (default: 1000).
     */
    void checkG2SEvents(uint32_t timeout = 1000)
    {
        G2SEvent::Data data{};

        mServiceKernel.app.unLock();
        bool status = mG2SQueue.pop(data, timeout);
        mServiceKernel.app.lock();

        if (!status) {
            return;
        }

        std::visit([this](const auto& event) {
            mServiceHandler.handleEvent(event);
        }, data);
    }

    /**
     * @brief Enqueue a GUI->Service (G2S) message.
     * @param data Message to push into @ref mG2SQueue.
     * @return @c true on success, @c false if the queue is full.
     */
    bool post(const G2SEvent::Data& data) override
    {
        return mG2SQueue.push(data);
    }

    /**
     * @brief Bind GUI‑side context used for S2G dispatching.
     *
     * @param kernel  Pointer to the GUI‑side kernel facade (used to unlock/lock around pops).
     * @param handler Pointer to the GUI handler that will receive dispatched events.
     *
     * @pre  @p kernel and @p handler (if non‑null) must remain valid for as long
     *       as @ref checkS2GEvents() may be called.
     */
    void setGUIHandler(const SDK::Kernel* kernel, IGUIModelHandler* handler) override
    {
        mGUIKernel  = kernel;
        mGUIHandler = handler;
    }

    /**
     * @brief Drain Service->GUI queue and dispatch to the GUI handler.
     *
     * Attempts to pop one S2G message from @ref mS2GQueue with an optional
     * timeout and, if successful, dispatches it to the GUI handler previously
     * bound via @ref IGUIModel::setGUIHandler().
     *
     * @param timeout Pop wait time in milliseconds (default: 0 for non-blocking).
     */
    void process(uint32_t timeout = 0) override
    {
        S2GEvent::Data data{};

        mGUIKernel->app.unLock();
        bool status = mS2GQueue.pop(data, timeout);
        mGUIKernel->app.lock();

        if (!status) {
            return;
        }

        if (mGUIHandler) {
            std::visit([this](const auto& event) {
                mGUIHandler->handleEvent(event);
            }, data);
        }
    }

    /**
     * @brief Enqueue a Service->GUI (S2G) message.
     * @param data Message to push into @ref mS2GQueue.
     * @return @c true on success, @c false if the queue is full.
     */
    bool sendToGUI(const S2GEvent::Data& data)
    {
        return mS2GQueue.push(data);
    }

private:
    const SDK::Kernel&                 mServiceKernel;  ///< Service‑side kernel facade.
    const SDK::Kernel*                 mGUIKernel;      ///< GUI‑side kernel facade (non‑owning; may be null until bound).
    CircularBuffer<S2GEvent::Data, 20> mS2GQueue;       ///< Ring buffer for Service->GUI traffic.
    CircularBuffer<G2SEvent::Data, 20> mG2SQueue;       ///< Ring buffer for GUI->Service traffic.
    IServiceModelHandler&              mServiceHandler; ///< Service‑side event handler (non‑owning).
    IGUIModelHandler*                  mGUIHandler;     ///< GUI‑side event handler (non‑owning; optional).
};

#endif // __GS_BRIDGE_HPP
