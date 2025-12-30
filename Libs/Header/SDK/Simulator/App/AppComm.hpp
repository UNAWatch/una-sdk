/**
 * @file   AppComm.hpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application communication (Service + GUI + Kernel)
 */

#ifndef __APP_COMM_HPP
#define __APP_COMM_HPP

#include "SDK/Interfaces/IAppComm.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/App/MessageManager.hpp"

namespace SDK::App
{

class Comm {
public:
    class IDispatch {
    public:
		virtual bool dispatchMessage(SDK::MessageBase* msg) = 0;  
    };

    // =========================================================================
    // Configuration constants
    // =========================================================================
    
    static constexpr size_t SERVICE_QUEUE_SIZE = 20;    // Kernel -> Service
    static constexpr size_t GUI_QUEUE_SIZE     = 20;    // Kernel -> GUI

    Comm(uint32_t               srvPID,
         uint32_t               guiPID,
         ::App::MessageManager& messageManager,
		 IDispatch&             dispatcher);
    
    ~Comm();
    
    // =========================================================================
    // IAppComm interface access (for Service and GUI threads)
    // =========================================================================
    
    SDK::Interface::IAppComm* getServiceComm();
    SDK::Interface::IAppComm* getGuiComm();
    
    // =========================================================================
    // Kernel-side API (common queue management)
    // =========================================================================
    
    bool sendToService(SDK::MessageBase* msg);
    bool sendToGui(SDK::MessageBase* msg);

    void clearServiceQueue();
    void clearGuiQueue();
    void clearAllQueues();
    
    // =========================================================================
    // Process identification
    // =========================================================================
    
    uint32_t getServicePID() const;
    uint32_t getGuiPID()     const;
    
private:
    // =========================================================================
    // Internal IAppComm implementation for Service and GUI
    // =========================================================================
    
    class InternalAppComm : public SDK::Interface::IAppComm {
    public:
        InternalAppComm(uint32_t                                          pid,
                        ::App::MessageManager&                            messageManager,
                        IDispatch&                                        dispatcher,
                        OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE>& serviceQueue,
                        OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>&     guiQueue,
                        bool                                              isService);
        
        uint32_t getProcessId() const                                                override;
        void     releaseMessage(SDK::MessageBase* msg)                               override;
        void*    allocateMessage(size_t size)                                        override;
        bool     sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs = 0)          override;
        bool     getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs = 0xFFFFFFFF) override;
        void     sendResponse(SDK::MessageBase* msg)                                 override;
        
    private:
        uint32_t                                          mPID;
        ::App::MessageManager&                            mMessageManager;
        IDispatch&                                        mDispatcher;
        OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE>& mServiceQueue;    // To Service
        OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>&     mGuiQueue;        // To GUI
        bool                                              mIsService;
    };
    
    // =========================================================================
    // Private members
    // =========================================================================
    const uint32_t         mServicePID;
    const uint32_t         mGuiPID;
    ::App::MessageManager& mMessageManager;
    
    // Queues
    OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE> mServiceQueue;     // Kernel -> Service
    OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>     mGuiQueue;         // Kernel -> GUI
    
    // Two IAppComm interfaces
    InternalAppComm mServiceComm;
    InternalAppComm mGuiComm;
    
    // Prevent copying
    Comm(const Comm&) = delete;
    Comm& operator=(const Comm&) = delete;
};

} // namespace App

#endif // __APP_COMM_HPP
