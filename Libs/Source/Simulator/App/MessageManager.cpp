/**
 * @file MessageManager.cpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Implementation of message lifecycle manager
 */

#include "SDK/Simulator/App/MessageManager.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <semaphore>
#include <chrono>
#include <cstring>

#define LOG_MODULE_PRX      "App.MessageManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace App
{

MessageManager::MessageManager()
{
}

bool MessageManager::initCompletionSemaphore(SDK::MessageBase* msg)
{
    if (msg == nullptr) {
        return false;
    }

    auto* sem = new OS::Semaphore(1, 0); // max=1, init=0

    msg->mCompletionSemaphore = static_cast<void*>(sem);

    msg->mNeedsResponse = true;
    msg->mCompleted.store(false);

    return true;
}

void* MessageManager::allocateRawMemory(size_t size)
{
    void* ptr = new uint8_t[size];

    if (ptr == nullptr) {
        LOG_ERROR("Failed to allocate memory: size %d\n", size);
    }

    return ptr;
}

void MessageManager::releaseMessage(SDK::MessageBase* msg)
{
    if (msg == nullptr) {
        return;
    }

    // Decrement reference count atomically
    uint32_t oldCount = msg->mRefCount.fetch_sub(1);

    #ifdef DEBUG
    // Detect double release
    if (oldCount == 0) {
        configASSERT(false);
    }
    #endif

    // Check if last reference
    if (oldCount == 1) {
        // Cleanup completion semaphore if exists
        cleanupCompletionSemaphore(msg);
        // operator delete is a no-op (pool allocator); this calls the virtual dtor once
        delete msg;
    }
}

void MessageManager::retainMessage(SDK::MessageBase* msg)
{
    if (msg == nullptr) {
        return;
    }

    msg->mRefCount.fetch_add(1);
}

bool MessageManager::waitCompletion(SDK::MessageBase* msg, uint32_t timeoutMs)
{
    if (!msg || !msg->mCompletionSemaphore)
        return false;

    auto* sem = static_cast<OS::Semaphore*>(msg->mCompletionSemaphore);
    return sem->take(timeoutMs);
}

void MessageManager::signalCompletion(SDK::MessageBase* msg)
{
    if (msg == nullptr || msg->mCompletionSemaphore == nullptr) {
        return;
    }

    msg->mCompleted.store(true, std::memory_order_release);

    auto* sem = static_cast<OS::Semaphore*>(msg->mCompletionSemaphore);
    sem->give();
}

void MessageManager::signalCompletion(SDK::MessageBase* msg, SDK::MessageResult result)
{
    if (msg == nullptr) {
        return;
    }
    msg->setResult(result);
    signalCompletion(msg);
}

void MessageManager::cleanupCompletionSemaphore(SDK::MessageBase* msg)
{
    if (msg == nullptr)
        return;

    auto* sem = static_cast<OS::Semaphore*>(msg->mCompletionSemaphore);
    delete sem;

    msg->mCompletionSemaphore = nullptr;
}

} // namespace App

