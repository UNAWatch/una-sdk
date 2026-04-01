/**
 * @file DualAppComm.cpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Dual-part application communication (Service + GUI)
 *
 */

#include "SDK/Simulator/App/DualAppComm.hpp"
#include "SDK/Simulator/OS/OS.hpp"

#define LOG_MODULE_PRX      "App.DualAppComm"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace SDK::App
{


    DualAppComm::DualAppComm(uint32_t servicePID,
        uint32_t guiPID,
        ::App::MessageManager& msgManager)
        : mServicePID(servicePID)
        , mGuiPID(guiPID)
        , mMessageManager(msgManager)
        , mCommonQueue()
        , mServiceQueue()
        , mGuiQueue()
        , mServiceComm(servicePID, msgManager, mCommonQueue, mServiceQueue, mGuiQueue, true)
        , mGuiComm(guiPID, msgManager, mCommonQueue, mServiceQueue, mGuiQueue, false)
    {
        mCommonQueue.init("AppCommon");
        mServiceQueue.init("AppService");
        mGuiQueue.init("AppGui");
    }

    DualAppComm::~DualAppComm()
    {
        clearAllQueues();
        mCommonQueue.deinit();
        mServiceQueue.deinit();
        mGuiQueue.deinit();
    }

    bool DualAppComm::receiveFromApp(SDK::MessageBase*& msg, uint32_t timeoutMs)
    {
        bool status = mCommonQueue.pop(msg, timeoutMs);
        if (msg) {
            LOG_DEBUG("Kernel received Message 0x%08X, from PID 0x%08X\n", msg->getType(), msg->getProcessId());
        }
        else {
            LOG_DEBUG("Kernel received Empty Message\n");
        }
        return status;
    }

    bool DualAppComm::sendToApp(SDK::MessageBase* msg, uint32_t timeoutMs)
    {
        if (msg == nullptr) {
            return false;
        }

        // Create completion semaphore if needed
        if (timeoutMs > 0) {
            if (!mMessageManager.initCompletionSemaphore(msg)) {
                LOG_ERROR("Failed to send internal Message to app. Invalid Semaphore.\n");
                return false;
            }
        }
        LOG_DEBUG("Backend -> Kernel.  Message 0x%08X\n", msg->getType());
        mMessageManager.retainMessage(msg);
        bool success = mCommonQueue.push(msg);

        if (!success) {
            // Send failed - cleanup semaphore
            if (msg->needsResponse()) {
                mMessageManager.cleanupCompletionSemaphore(msg);
            }
            mMessageManager.releaseMessage(msg);
            LOG_ERROR("Failed to send internal Message to app. Queue is full.\n");
            return false;
        }

        if (msg->needsResponse()) {
            bool completed = mMessageManager.waitCompletion(msg, timeoutMs);

            if (!completed) {
                // Timeout - message was sent but no response received
                msg->setResult(SDK::MessageResult::TIMEOUT);
                // Return true because send was successful (only response timed out)
            }
        }

        return true;
    }

    bool DualAppComm::sendToService(SDK::MessageBase* msg)
    {
        if (msg == nullptr) {
            return false;
        }
        uint32_t timeoutMs = 0; // avoid deadlock
        // Create completion semaphore if needed
        if (timeoutMs > 0) {
            if (!mMessageManager.initCompletionSemaphore(msg)) {
                LOG_ERROR("Failed to send Message 0x%08X to PID 0x%08X. Invalid Semaphore.\n", msg->getType(), mServicePID);
                return false;
            }
        }

        LOG_DEBUG("Kernel  -> Service. Message 0x%08X to PID 0x%08X.\n", msg->getType(), mServicePID);
        mMessageManager.retainMessage(msg);
        bool success = mServiceQueue.push(msg);

        if (!success) {
            // Send failed - cleanup semaphore
            if (msg->needsResponse()) {
                mMessageManager.cleanupCompletionSemaphore(msg);
            }
            mMessageManager.releaseMessage(msg);
            LOG_ERROR("Failed to send Message 0x%08X to PID 0x%08X. Queue is full. %u\n", msg->getType(), mServicePID, mServiceQueue.size());
            return false;
        }

        if (msg->needsResponse()) {
            bool completed = mMessageManager.waitCompletion(msg, timeoutMs);

            if (!completed) {
                // Timeout - message was sent but no response received
                msg->setResult(SDK::MessageResult::TIMEOUT);
                // Return true because send was successful (only response timed out)
            }
        }

        return true;
    }

    bool DualAppComm::sendToGui(SDK::MessageBase* msg)
    {
        if (msg == nullptr) {
            return false;
        }
        uint32_t timeoutMs = 0; // avoid deadlock
        // Create completion semaphore if needed
        if (timeoutMs > 0) {
            if (!mMessageManager.initCompletionSemaphore(msg)) {
                LOG_ERROR("Failed to send Message 0x%08X to PID 0x%08X. Invalid Semaphore.\n", msg->getType(), mGuiPID);
                return false;
            }
        }

        LOG_DEBUG("Kernel  -> GUI.     Message 0x%08X to PID 0x%08X.\n", msg->getType(), mGuiPID);
        mMessageManager.retainMessage(msg);
        bool success = mGuiQueue.push(msg);

        if (!success) {
            // Send failed - cleanup semaphore
            if (msg->needsResponse()) {
                mMessageManager.cleanupCompletionSemaphore(msg);
            }
            mMessageManager.releaseMessage(msg);
            LOG_ERROR("Failed to send Message 0x%08X to PID 0x%08X. Queue is full.\n", msg->getType(), mGuiPID);
            return false;
        }

        if (msg->needsResponse()) {
            bool completed = mMessageManager.waitCompletion(msg, timeoutMs);

            if (!completed) {
                // Timeout - message was sent but no response received
                msg->setResult(SDK::MessageResult::TIMEOUT);
                // Return true because send was successful (only response timed out)
            }
        }

        return true;
    }

    void DualAppComm::clearServiceQueue()
    {
        SDK::MessageBase* msg;
        while (mServiceQueue.pop(msg, 0)) {
            LOG_WARNING("Clear Service Message 0x%08X, PID 0x%08X.\n", msg->getType(), getServicePID());
            mMessageManager.releaseMessage(msg);
        }
    }

    void DualAppComm::clearGuiQueue()
    {
        SDK::MessageBase* msg;
        while (mGuiQueue.pop(msg, 0)) {
            LOG_WARNING("Clear GUI Message 0x%08X, PID 0x%08X.\n", msg->getType(), getGuiPID());
            mMessageManager.releaseMessage(msg);
        }
    }

    void DualAppComm::clearCommonQueue()
    {
        SDK::MessageBase* msg;
        while (mCommonQueue.pop(msg, 0)) {
            LOG_WARNING("Clear Common Message 0x%08X\n", msg->getType());
            mMessageManager.releaseMessage(msg);
        }
    }

    void DualAppComm::clearAllQueues()
    {
        // Clear GUI queue
        clearGuiQueue();

        // Clear Service queue
        clearServiceQueue();

        // Clear common queue
        clearCommonQueue();

        LOG_INFO("Queues cleared\n");
    }

    // =============================================================================
    // InternalAppComm implementation
    // =============================================================================

    bool DualAppComm::InternalAppComm::sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs)
    {
        if (msg == nullptr) {
            return false;
        }

        // Set process ID
        msg->setProcessId(mPID);

        // Create completion semaphore if needed
        if (timeoutMs > 0) {
            if (!mMessageManager.initCompletionSemaphore(msg)) {
                LOG_ERROR("Failed to send Message 0x%08X from PID 0x%08X. Invalid Semaphore.\n", msg->getType(), mPID);
                return false;
            }
        }

        // Check if message is internal (Service <-> GUI direct routing)
        SDK::MessageType::Type msgType = msg->getType();
        bool success;
        mMessageManager.retainMessage(msg);

        if (SDK::isApplicationSpecificMessage(msgType)) {
            // Internal message - route directly to other part
            if (mIsService) {
                // Service -> GUI direct
                LOG_DEBUG("Service -> GUI.     Message 0x%08X from PID 0x%08X.\n", msg->getType(), mPID);
                success = mGuiQueue.push(msg);
            }
            else {
                // GUI -> Service direct
                LOG_DEBUG("GUI     -> Service. Message 0x%08X from PID 0x%08X.\n", msg->getType(), mPID);
                success = mServiceQueue.push(msg);
            }
        }
        else {
            // Regular message - send to kernel via common queue
            LOG_DEBUG("%s -> Kernel.  Message 0x%08X from PID 0x%08X.\n", mIsService ? "Service" : "GUI    ", msg->getType(), mPID);
            success = mCommonQueue.push(msg);
        }

        if (!success) {
            // Send failed - cleanup semaphore
            if (msg->needsResponse()) {
                mMessageManager.cleanupCompletionSemaphore(msg);
            }
            mMessageManager.releaseMessage(msg);
            LOG_ERROR("Failed to send Message 0x%08X from PID 0x%08X. Queue is full.\n", msg->getType(), mPID);
            return false;
        }

        if (msg->needsResponse()) {
            bool completed = mMessageManager.waitCompletion(msg, timeoutMs);

            if (!completed) {
                // Timeout - message was sent but no response received
                msg->setResult(SDK::MessageResult::TIMEOUT);
                // Return true because send was successful (only response timed out)
            }
        }

        return true;
    }

    bool DualAppComm::InternalAppComm::getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs)
    {
        // Service reads from Service queue, GUI reads from GUI queue
        bool status = false;
        if (mIsService) {
            status = mServiceQueue.pop(msg, timeoutMs);
            if (status) {
                if (msg) {
                    LOG_DEBUG("Service received Message 0x%08X\n", msg->getType());
                }
                else {
                    LOG_DEBUG("Service received Empty Message\n");
                }
            }
        }
        else {
            status = mGuiQueue.pop(msg, timeoutMs);
            if (status) {
                if (msg) {
                    LOG_DEBUG("GUI received Message 0x%08X\n", msg->getType());
                }
                else {
                    LOG_DEBUG("GUI received Empty Message\n");
                }
            }
        }
        return status;
    }

    void DualAppComm::InternalAppComm::sendResponse(SDK::MessageBase* msg)
    {
        if (msg == nullptr || !msg->needsResponse()) {
            return;
        }

        mMessageManager.signalCompletion(msg);
    }

} // namespace App
