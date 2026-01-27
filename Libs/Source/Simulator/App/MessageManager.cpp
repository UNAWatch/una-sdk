/**
 * @file MessageManager.cpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Implementation of message lifecycle manager
 */

#include "SDK/Simulator/App/MessageManager.hpp"

#include <cstring>

#define LOG_MODULE_PRX      "App.MessageManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace App
{

MessageManager::MessageManager()
{
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
        // Call virtual destructor
        msg->~MessageBase();

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

void MessageManager::signalCompletion(SDK::MessageBase* msg, SDK::MessageResult result)
{
    if (msg == nullptr) {
        return;
    }
    msg->setResult(result);
    msg->mCompleted.store(true);
}

} // namespace App

