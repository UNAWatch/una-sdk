/**
 * @file   AppComm.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application communication (Service + GUI + Kernel)
 *
 */

#include "SDK/Simulator/App/AppComm.hpp"

#define LOG_MODULE_PRX      "AppComm"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace SDK::App
{
Comm::Comm(uint32_t               srvPID,
           uint32_t               guiPID,
           ::App::MessageManager& messageManager,
           IDispatch&             dispatcher)
    : mServicePID(srvPID)
	, mGuiPID(guiPID)
    , mMessageManager(messageManager)
    , mServiceQueue()
    , mGuiQueue()
    , mServiceComm(mServicePID,
                   mMessageManager,
                   dispatcher,
                   mServiceQueue,
                   mGuiQueue,
                   true)
    , mGuiComm(mGuiPID,
               mMessageManager,
               dispatcher,
               mServiceQueue,
               mGuiQueue,
               false)
{
    mServiceQueue.init("AppService");
    mGuiQueue.init("AppGui");
}

Comm::~Comm()
{
    clearAllQueues();

    mServiceQueue.deinit();
    mGuiQueue.deinit();
}

SDK::Interface::IAppComm* Comm::getServiceComm()
{
    return &mServiceComm;
}

SDK::Interface::IAppComm* Comm::getGuiComm()
{
    return &mGuiComm;
}

bool Comm::sendToService(SDK::MessageBase* msg/*, uint32_t timeoutMs*/)
{
    if (msg == nullptr) {
        return false;
    }

    mMessageManager.retainMessage(msg);
    bool success = mServiceQueue.push(msg);

    if (!success) {
        // Send failed - cleanup semaphore
        mMessageManager.releaseMessage(msg);
        LOG_ERROR("Failed to send Message to PID 0x%08X. Queue is full.\n", mServicePID);
        return false;
    }

    return true;
}

bool Comm::sendToGui(SDK::MessageBase* msg/*, uint32_t timeoutMs*/)
{
    if (msg == nullptr) {
        return false;
    }

    mMessageManager.retainMessage(msg);
    bool success = mGuiQueue.push(msg);

    if (!success) {
        // Send failed - cleanup semaphore
        mMessageManager.releaseMessage(msg);
        LOG_ERROR("Failed to send Message to PID 0x%08X. Queue is full.\n", mGuiPID);
        return false;
    }

    return true;
}

void Comm::clearServiceQueue()
{
    SDK::MessageBase* msg;
    while (mServiceQueue.pop(msg)) {
        mMessageManager.releaseMessage(msg);
    }
}

void Comm::clearGuiQueue()
{
    SDK::MessageBase* msg;
    while (mGuiQueue.pop(msg)) {
        mMessageManager.releaseMessage(msg);
    }
}

void Comm::clearAllQueues()
{
    // Clear GUI queue
    clearGuiQueue();

    // Clear Service queue
    clearServiceQueue();
}

// =========================================================================
// Process identification
// =========================================================================

uint32_t Comm::getServicePID() const
{ 
    return mServicePID;
}

uint32_t Comm::getGuiPID() const
{
    return mGuiPID;
}

// =============================================================================
// InternalAppComm implementation
// =============================================================================

Comm::InternalAppComm::InternalAppComm(uint32_t                                          pid,
                                       ::App::MessageManager&                            messageManager,
                                       IDispatch&                                        dispatcher,
                                       OS::Queue<SDK::MessageBase*, SERVICE_QUEUE_SIZE>& serviceQueue,
                                       OS::Queue<SDK::MessageBase*, GUI_QUEUE_SIZE>&     guiQueue,
                                       bool                                              isService)
    : mPID(pid)
    , mMessageManager(messageManager)
    , mDispatcher(dispatcher)
    , mServiceQueue(serviceQueue)
    , mGuiQueue(guiQueue)
    , mIsService(isService)
{}

uint32_t Comm::InternalAppComm::getProcessId() const
{
    return mPID;
}

void Comm::InternalAppComm::releaseMessage(SDK::MessageBase* msg)
{
    mMessageManager.releaseMessage(msg);
}

void* Comm::InternalAppComm::allocateMessage(size_t size)
{
    return mMessageManager.allocateRawMemory(size);
}

bool Comm::InternalAppComm::sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs)
{
    if (msg == nullptr) {
        return false;
    }
    
    // Set process ID
    msg->setProcessId(mPID);
    
    // Check if message is internal (Service <-> GUI direct routing)
    SDK::MessageType::Type msgType = msg->getType();
    bool success;
    mMessageManager.retainMessage(msg);
    
    if (SDK::isApplicationSpecificMessage(msgType)) {
        // Internal message - route directly to other part
        if (mIsService) {
            // Service -> GUI direct
            success = mGuiQueue.push(msg);
        } else {
            // GUI -> Service direct
            success = mServiceQueue.push(msg);
        }
    } else {
		// Regular message - dispatch kernel's message
		success = mDispatcher.dispatchMessage(msg);

		if (success) {
            mMessageManager.releaseMessage(msg);
            LOG_DEBUG("Message 0x%08X released \n", msg->getType());
        }
    }
    
    if (!success) {
        mMessageManager.releaseMessage(msg);
        LOG_ERROR("Failed to send Message 0x%08X from PID 0x%08X. Queue is full.\n",  msg->getType(), mPID);
        return false;
    }
    
    return true;
}

bool Comm::InternalAppComm::getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs)
{
    // Service reads from Service queue, GUI reads from GUI queue
    if (mIsService) {
        return mServiceQueue.pop(msg);
    } else {
        return mGuiQueue.pop(msg);
    }
}

void Comm::InternalAppComm::sendResponse(SDK::MessageBase* msg)
{
    (void) msg;
}

} // namespace SDK::App
