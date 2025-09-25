/**
 ******************************************************************************
 * @file    GSModel.hpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Generic GUI↔Service model with bidirectional event queues.
 * @details This header defines a templated mediator @c GSModel that bridges a
 *          GUI thread/task and a Service thread/task using two lock-free style
 *          circular queues:
 *            - G2S: GUI → Service events
 *            - S2G: Service → GUI events
 *
 *          The model integrates with the SDK kernel facade to perform brief
 *          unlock/lock sections around queue operations (so the opposite side
 *          can progress) and dispatches events via user-provided handler types
 *          using @c std::visit on @c std::variant payloads.
 *
 * @tparam S2G  Variant (or POD) type for Service→GUI events.
 * @tparam G2S  Variant (or POD) type for GUI→Service events.
 * @tparam SH   Service-side handler type; must provide
 *              <tt>void handleEvent(const X&)</tt> for each alternative of @p G2S.
 * @tparam GH   GUI-side handler type; must provide
 *              <tt>void handleEvent(const X&)</tt> for each alternative of @p S2G.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GS_MODEL_HPP
#define __GS_MODEL_HPP

#include "SDK/GSModel/IGUIModel.hpp"
#include "SDK/CircularBuffer.hpp"
#include "SDK/AppSystem/AppKernel.hpp"

#include <type_traits>
#include <variant>

/**
 * @brief GUI↔Service model that queues and dispatches events in both directions.
 *
 * @details The class owns two ring buffers:
 *          - @ref mG2SQueue for GUI→Service traffic (written by GUI, read by Service)
 *          - @ref mS2GQueue for Service→GUI traffic (written by Service, read by GUI)
 *
 *          Dispatching is performed with @c std::visit, calling the corresponding
 *          @c handleEvent overload on the bound handler object.
 *
 * @note    Queue capacity is fixed by the @c CircularBuffer template parameter (here 20).
 * @warning The brief @c app.unLock()/@c app.lock() sections assume cooperative scheduling
 *          in your environment. Adjust to your kernel’s critical-section model if needed.
 */
template<typename S2G, typename G2S, typename SH, typename GH>
class GSModel : public IGUIModel<G2S, GH>
{
public:
    /**
     * @brief Construct the model and initialize queues.
     * @param handler Reference to the Service-side handler implementation.
     *
     * @post Queues are initialized; the Service kernel facade is cached.
     */
    GSModel(SH& handler)
        : mServiceKernel(SDK::Kernel::GetInstance())
        , mS2GQueue(mServiceKernel)
        , mG2SQueue(mServiceKernel)
        , mServiceHandler(handler)
    {
        mS2GQueue.init();
        mG2SQueue.init();
    }

    /**
     * @brief Virtual destructor (no-op).
     */
    ~GSModel() override = default;

    /**
     * @brief Drain GUI→Service queue and dispatch to the Service handler.
     *
     * Attempts to pop one G2S message from @ref mG2SQueue with an optional
     * timeout and, if successful, dispatches it to @ref mServiceHandler via
     * @c std::visit.
     *
     * @param timeout Pop wait time in milliseconds (default: 1000).
     */
    void checkG2SEvents(uint32_t timeout = 1000)
    {
        G2S data{};

        mServiceKernel.app.unLock();
        bool status = mG2SQueue.pop(data, timeout);
        mServiceKernel.app.lock();

        if (!status) return;

        std::visit([this](const auto& event) {
            mServiceHandler.handleEvent(event);
        }, data);
    }

    /**
     * @brief Enqueue a GUI→Service (G2S) message.
     * @param data Message to push into @ref mG2SQueue.
     * @return @c true on success, @c false if the queue is full.
     */
    bool sendToService(const G2S& data) override
    {
        return mG2SQueue.push(data);
    }

    /**
     * @brief Drain Service→GUI queue and dispatch to the GUI handler.
     *
     * Attempts to pop one S2G message from @ref mS2GQueue with an optional
     * timeout and, if successful, dispatches it to the GUI handler previously
     * bound via @ref IGUIModel::setGUIHandler().
     *
     * @param timeout Pop wait time in milliseconds (default: 0 for non-blocking).
     */
    void checkS2GEvents(uint32_t timeout = 0) override
    {
        S2G data{};

        this->mGUIKernel->app.unLock();
        bool status = mS2GQueue.pop(data, timeout);
        this->mGUIKernel->app.lock();

        if (!status) return;

        if (this->mGUIHandler) {
            std::visit([this](const auto& event) {
                this->mGUIHandler->handleEvent(event);
            }, data);
        }
    }

    /**
     * @brief Enqueue a Service→GUI (S2G) message.
     * @param data Message to push into @ref mS2GQueue.
     * @return @c true on success, @c false if the queue is full.
     */
    bool sendToGUI(const S2G& data)
    {
        return mS2GQueue.push(data);
    }

private:
    /**
     * @brief Reference to the Service-side kernel facade (non-owning).
     */
    const SDK::Kernel&      mServiceKernel;

    /**
     * @brief Service→GUI queue (produced by Service, consumed by GUI).
     */
    CircularBuffer<S2G, 20> mS2GQueue;

    /**
     * @brief GUI→Service queue (produced by GUI, consumed by Service).
     */
    CircularBuffer<G2S, 20> mG2SQueue;

    /**
     * @brief Reference to the Service-side handler (non-owning).
     */
    SH&                     mServiceHandler;
};

#endif // __GS_MODEL_HPP
