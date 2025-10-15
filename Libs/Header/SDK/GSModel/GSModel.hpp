/**
 ******************************************************************************
 * @file    GSModel.hpp
 * @date    25-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   RAII facade for GUI↔Service bridge.
 *
 * @details Owns the underlying GSBridge and registers it in kernel.sctrl on
 *          construction. Exposes thin, intention-revealing proxies for:
 *            • checkG2SEvents() — poll GUI->Service and dispatch to Service
 *            • checkS2GEvents() — poll Service->GUI and dispatch to GUI
 *            • sendToService()  — enqueue GUI->Service event
 *            • sendToGUI()      — enqueue Service->GUI event
 ******************************************************************************
 */

#ifndef __GS_MODEL_HPP
#define __GS_MODEL_HPP

#include "SDK/GSModel/GSBridge.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"

/**
 * @brief Thin facade that registers and proxies a GUI <-> Service bridge.
 *
 * @details This class centralizes registration in kernel.sctrl and forwards
 *          calls to the underlying implementation. It does not manage queues
 *          or dispatching logic itself.
 */
class GSModel
{
public:
    /**
     * @brief Construct the facade and register the bridge in sctrl.
     * @param serviceHandler Service-side event handler.
     *
     * Retrieves the Service kernel via
     * SDK::KernelProviderService::GetInstance().getKernel(), creates the
     * underlying bridge, and registers it in kernel.sctrl.
     */
    GSModel(IServiceModelHandler& serviceHandler)
        : mGSBridge(std::make_shared<GSBridge>(serviceHandler))
    {
        auto& kernel = SDK::KernelProviderService::GetInstance().getKernel();

        kernel.sctrl.setContext(mGSBridge);
    }

    /**
     * @brief Poll GUI->Service events and dispatch to the Service handler.
     * @param timeout Wait time in milliseconds (default: 1000).
     *
     * @sa GSBridge::checkG2SEvents()
     */
    void process(uint32_t timeout = 1000)
    {
        mGSBridge->checkG2SEvents(timeout);
    }

    /**
     * @brief Publish a dummy event to your own queue to break the wait timeout.
     */
    void abortProcessWait()
    {
        // 'Default' is "empty" event type.
        // This type is always required. It is typically used
        // here to interrupt waiting on a queue.
        //
        // Add file G2SEvents.hpp to the project.
        // Typically this file should be located in: 'GSModelEvents/G2SEvents.hpp'
        // see template:
        // {SDK location}/Libs/Header/SDK/Templates/GSModelEvents/G2SEvents.hpp
        mGSBridge->post(G2SEvent::Default{});
    }

    /**
     * @brief Enqueue a Service->GUI (S2G) event.
     * @param data Event payload.
     * @retval true  Event enqueued.
     * @retval false Queue is full.
     *
     * @sa GSBridge::sendToGUI()
     */
    bool post(const S2GEvent::Data& data)
    {
        return mGSBridge->sendToGUI(data);
    }

private:
    std::shared_ptr<GSBridge> mGSBridge;
};

#endif // __GS_MODEL_HPP
