/**
 * @file DualAppComm.hpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Dual-part application communication (Service + GUI)
 * 
 * Manages communication for applications consisting of two parts:
 * - Service (background logic)
 * - GUI (user interface)
 * 
 * Architecture:
 * - 3 queues: Common (both->kernel), Service (kernel->srv), GUI (kernel->gui)
 * - Internal message routing between Service <-> GUI based on message type
 * - Single processing thread handles common queue
 * 
 * Internal message types (0x0000_8000 - 0x0000_8FFF):
 * - Used for direct Service <-> GUI communication
 * - Bypasses kernel processing
 * - Routes automatically based on type range
 */

#ifndef __APP_DUAL_APP_COMM_HPP
#define __APP_DUAL_APP_COMM_HPP

#include "SDK/Interfaces/IAppComm.hpp"
#include "SDK/Messages/MessageTypes.hpp"

 #include "SDK/Simulator/OS/OS.hpp"
 #include "SDK/Simulator/App/MessageManager.hpp"

namespace SDK::App
{

/**
 * @brief Dual-part application communication manager
 * 
 * Manages Service + GUI as single application with:
 * - 2 receive queues (Service, GUI)
 * - 1 common send queue (both -> kernel)
 * - Internal routing (Service <-> GUI direct) based on message type
 * 
 * Internal message type range: 0x0000_8000 - 0x0000_8FFF
 * Messages in this range route directly between Service and GUI
 */
class DualAppComm {
public:
    // =========================================================================
    // Configuration constants
    // =========================================================================
    
    static constexpr size_t COMMON_QUEUE_SIZE = 40;   // Both parts -> kernel
    static constexpr size_t SERVICE_QUEUE_SIZE = 20;  // Kernel -> Service
    static constexpr size_t GUI_QUEUE_SIZE = 20;      // Kernel -> GUI
    
    /**
     * @brief Construct dual application communication
     * @param servicePID Service process identifier
     * @param guiPID GUI process identifier
     * @param msgManager Reference to message manager
     * 
     * @note PIDs must be unique and non-zero
     * @note All queues initialized automatically
     */
    DualAppComm(uint32_t servicePID, 
                uint32_t guiPID, 
                ::App::MessageManager& msgManager);
    
    /**
     * @brief Destructor
     * @note Cleans up all queues
     */
    ~DualAppComm();
    
    ::App::MessageManager& getMsgManager()
    {
        return mMessageManager;
    }

    // =========================================================================
    // IAppComm interface access (for Service and GUI threads)
    // =========================================================================
    
    /**
     * @brief Get Service communication interface
     * @retval Pointer to IAppComm for Service thread
     */
    SDK::Interface::IAppComm* getServiceComm() { return &mServiceComm; }
    
    /**
     * @brief Get GUI communication interface
     * @retval Pointer to IAppComm for GUI thread
     */
    SDK::Interface::IAppComm* getGuiComm() { return &mGuiComm; }
    
    /**
     * @brief Get ICancellable interface to interrupt main thread.
     * @retval Pointer to ICancellable for thread interruption.
     */
   // OS::ICancellable* getAppCancellable() {return &mCommonQueue; }

    // =========================================================================
    // Kernel-side API (common queue management)
    // =========================================================================
    
    /**
     * @brief Receive message from application (Service or GUI)
     * @param msg Reference to message pointer (output)
     * @param timeoutMs Timeout in milliseconds
     * @retval true Message received successfully
     * @retval false Timeout or error
     * 
     * @note Reads from common queue (both Service and GUI send here)
     * @note Use msg->getProcessId() to determine sender
     */
    bool receiveFromApp(SDK::MessageBase*& msg, uint32_t timeoutMs);
    
    /**
     * @brief Send message to common queue for internal purposes
     * @param msg Message to send
     * @param timeoutMs Timeout in milliseconds (default 10ms)
     * @retval true Message sent successfully
     * @retval false Queue full or othe error
     *
     * @note Routes to Service or GUI based on msg->getProcessId()
     * @note Returns false if PID doesn't match Service or GUI
     */
    bool sendToApp(SDK::MessageBase* msg, uint32_t timeoutMs = 0);

    /**
     * @brief Send message to Service
     * @param msg Message to send
     * @param timeoutMs Timeout in milliseconds (default 10ms)
     * @retval true Message sent successfully
     * @retval false Queue full or timeout
     * 
     * @note Places message in Service receive queue
     */
    bool sendToService(SDK::MessageBase* msg/*, uint32_t timeoutMs = 0*/);
    
    /**
     * @brief Send message to GUI
     * @param msg Message to send
     * @param timeoutMs Timeout in milliseconds (default 10ms)
     * @retval true Message sent successfully
     * @retval false Queue full or timeout
     * 
     * @note Places message in GUI receive queue
     */
    bool sendToGui(SDK::MessageBase* msg/*, uint32_t timeoutMs = 0*/);

    /**
     * @brief Clear service queue and return messages to pool
     */
    void clearServiceQueue();

    /**
     * @brief Clear GUI queue and return messages to pool
     */
    void clearGuiQueue();

    /**
     * @brief Clear App queue and return messages to pool
     */
    void clearCommonQueue();

    /**
     * @brief Clear all queues and return messages to pool
     * 
     * @note Drains all three queues (common, Service, GUI)
     * @note Releases all messages back to pool
     * @note Useful during shutdown or reset
     */
    void clearAllQueues();

    // =========================================================================
    // Process identification
    // =========================================================================
    
    /**
     * @brief Get Service process ID
     * @retval Service PID
     */
    uint32_t getServicePID() const { return mServicePID; }
    
    /**
     * @brief Get GUI process ID
     * @retval GUI PID
     */
    uint32_t getGuiPID() const { return mGuiPID; }
    
private:
    // =========================================================================
    // Internal IAppComm implementation for Service and GUI
    // =========================================================================
    
    class InternalAppComm : public SDK::Interface::IAppComm {
    public:
        InternalAppComm(uint32_t pid, 
                       ::App::MessageManager& msgMgr,
                       OS::Queue<SDK::MessageBase*, COMMON_QUEUE_SIZE>& commonQueue,
                       OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE>& serviceQueue,
                       OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>& guiQueue,
                       bool isService)
            : mPID(pid)
            , mMessageManager(msgMgr)
            , mCommonQueue(commonQueue)
            , mServiceQueue(serviceQueue)
            , mGuiQueue(guiQueue)
            , mIsService(isService)
        {}
        
        uint32_t getProcessId() const override { return mPID; }
        
        void releaseMessage(SDK::MessageBase* msg) override {
            mMessageManager.releaseMessage(msg);
        }
        
        void* allocateMessage(size_t size) override {
            return mMessageManager.allocateRawMemory(size);
        }
        
        bool sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs = 0) override;
        bool getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs = 0xFFFFFFFF) override;
        void sendResponse(SDK::MessageBase* msg) override;
        
    private:
        uint32_t mPID;
        ::App::MessageManager& mMessageManager;
        OS::Queue<SDK::MessageBase*, COMMON_QUEUE_SIZE>& mCommonQueue;   // To kernel
        OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE>& mServiceQueue; // From/to Service
        OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>& mGuiQueue;         // From/to GUI
        bool mIsService;  // true if Service, false if GUI
    };
    
    // =========================================================================
    // Private members
    // =========================================================================
    
    const uint32_t mServicePID;
    const uint32_t mGuiPID;
    :: App::MessageManager& mMessageManager;
    
    // Three queues
    OS::Queue<SDK::MessageBase*, COMMON_QUEUE_SIZE> mCommonQueue;   // Service/GUI -> Kernel
    OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE> mServiceQueue;  // Kernel -> Service
    OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE> mGuiQueue;          // Kernel -> GUI
    
    // Two IAppComm interfaces
    InternalAppComm mServiceComm;
    InternalAppComm mGuiComm;
    
    // Prevent copying
    DualAppComm(const DualAppComm&) = delete;
    DualAppComm& operator=(const DualAppComm&) = delete;
};

} // namespace App

#endif // __APP_DUAL_APP_COMM_HPP
