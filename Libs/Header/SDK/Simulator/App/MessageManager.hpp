/**
 * @file MessageManager.hpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Message lifecycle management and pool allocation
 *
 * MessageManager handles:
 * - Memory allocation from pools
 * - Reference counting
 * - Completion synchronization (semaphores)
 * - Message lifecycle (allocation to deallocation)
 *
 */

#ifndef __APP_MESSAGE_MANAGER_HPP
#define __APP_MESSAGE_MANAGER_HPP

#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"

namespace App
{

/**
 * @brief Message lifecycle manager
 *
 * Manages message allocation, reference counting, and cleanup.
 * Uses MessagePool for memory allocation.
 * Handles completion synchronization for request-response patterns.
 */
class MessageManager {
public:
    /**
     * @brief Construct message manager
     */
    MessageManager();

    /**
     * @brief Destructor
     */
    ~MessageManager() = default;

    // =========================================================================
    // Message allocation and deallocation
    // =========================================================================

    /**
     * @brief Allocate raw memory from pool
     * @param size Required message size in bytes
     * @retval Pointer to allocated raw memory on success
     * @retval nullptr if allocation failed
     * @note Memory is zeroed
     * @note Caller must use placement new to construct message
     */
    void* allocateRawMemory(size_t size);

    /**
     * @brief Allocate typed message object from pool
     * @retval Pointer to typed message on success
     * @retval nullptr if allocation failed
     *
     * @note Template wrapper for type-safe allocation
     * @note Calls placement new on allocated memory
     * @note Pool determined automatically by address during deallocation
     * @note Example: auto* msg = msgMgr.allocateMessage<GpsRequestMsg>();
     */
    template<typename T>
    T* allocateMessage() {
        void* ptr = allocateRawMemory(sizeof(T));
        if (ptr) {
            // Placement new to construct typed object
            return new (ptr) T();
        }
        return nullptr;
    }

    /**
     * @brief Release message back to pool
     * @param msg Message to release
     * @note Decrements reference count
     * @note Returns to pool when refCount reaches zero
     * @note Cleans up completion semaphore if exists
     * @note Thread-safe operation
     */
    void releaseMessage(SDK::MessageBase* msg);

    /**
     * @brief Retain message (increment reference count)
     * @param msg Message to retain
     * @note Thread-safe operation
     */
    void retainMessage(SDK::MessageBase* msg);

    void signalCompletion(SDK::MessageBase* msg, SDK::MessageResult result);

private:
    // Prevent copying
    MessageManager(const MessageManager&) = delete;
    MessageManager& operator=(const MessageManager&) = delete;
};

} // namespace App

#endif // __APP_MESSAGE_MANAGER_HPP
