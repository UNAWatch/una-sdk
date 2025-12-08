/**
 * @file    MessageBase.hpp
 * @date    03-12-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Base class for all messages exchanged between kernel and applications
 *
 * This file defines the base message class with reference counting,
 * completion synchronization, and pool management. All concrete message
 * types must inherit from MessageBase.
 *
 * Shared between kernel and applications.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

#include "SDK/Messages/MessageTypes.hpp"

// Forward declarations
namespace App
{
    class MessageManager;
}

namespace SDK
{

/**
 * @brief Message processing result
 */
enum class MessageResult : uint8_t {
    PENDING = 0,        // Message not yet processed
    SUCCESS = 1,        // Processing completed successfully
    ERROR   = 2,        // Processing failed
    TIMEOUT = 3         // Request timed out (no response received)
};

/**
 * @brief Base class for all messages
 *
 * Provides:
 * - Type identification for message dispatch
 * - Reference counting for safe shared ownership
 * - Completion synchronization for request-response pattern
 * - Result status tracking
 *
 * Memory lifecycle:
 * 1. Allocated from pool (refCount = 1)
 * 2. Retained when placed in queue (refCount++)
 * 3. Released after processing (refCount--)
 * 4. Returned to pool when refCount reaches 0
 */
class MessageBase {
public:
    // =========================================================================
    // User API - Getters (read message state)
    // =========================================================================

    /**
     * @brief Get message type
     * @retval Message type identifier
     */
    MessageType::Type getType() const { return mMsgType; }

    /**
     * @brief Get processing result
     * @retval Current result status
     */
    MessageResult getResult() const { return mResult; }

    /**
     * @brief Get processing result as string
     * @retval Current result status
     */
    const char* getResultStr() const {
        switch (mResult) {
            case MessageResult::PENDING: return "PENDING";
            case MessageResult::SUCCESS: return "SUCCESS";
            case MessageResult::ERROR:   return "ERROR";
            case MessageResult::TIMEOUT: return "TIMEOUT";
        }
        return "UNKNOWN";
    }

    /**
     * @brief Get process ID (owner or target)
     * @retval Process identifier assigned by kernel
     */
    uint32_t getProcessId() const { return mProcessId; }

    /**
     * @brief Check if message expects response
     * @retval true Response expected (request-response pattern)
     * @retval false Fire-and-forget message
     */
    bool needsResponse() const { return mNeedsResponse; }

    /**
     * @brief Check if message processing completed
     * @retval true Processing completed (response ready or error occurred)
     * @retval false Still pending
     */
    bool isCompleted() const { return mCompleted.load(); }

    /**
     * @brief Get current reference count
     * @retval Number of active references to this message
     * @note For debugging purposes only
     */
    uint32_t getRefCount() const { return mRefCount.load(); }


    // =========================================================================
    // Kernel API - Message processing
    // =========================================================================

    /**
     * @brief Set processing result
     * @param result Result status
     * @note Called by message processor (kernel or application)
     */
    void setResult(MessageResult result) { mResult = result; }

    /**
     * @brief Set process ID
     * @param processId Process identifier to set
     * @note Called by AppComm::sendMessage() before routing
     */
    void setProcessId(uint32_t processId) { mProcessId = processId; }

    // =========================================================================
    // Memory allocation control (placement new for pool allocation)
    // =========================================================================

    /**
     * @brief Delete direct heap allocation
     * @note Messages can only be allocated through kernel pools
     */
    static void* operator new(size_t) = delete;
    static void* operator new[](size_t) = delete;

    /**
     * @brief Allow placement new (used by kernel infrastructure)
     * @param size Size of object
     * @param ptr Pre-allocated memory pointer
     * @retval Pointer to placement location
     */
    static void* operator new(size_t size, void* ptr) noexcept {
        (void)size;
        return ptr;
    }

    /**
     * @brief Delete operator - no-op (memory managed by pool)
     * @note Cannot delete messages directly, must use releaseMessage()
     */
    static void operator delete(void*) noexcept {
        // No-op: memory returned to pool by MessageManager
    }

    /**
     * @brief Placement delete operator (for exception safety)
     * @param ptr Memory pointer
     * @param place Placement pointer
     * @note Called if placement new throws exception
     */
    static void operator delete(void* ptr, void* place) noexcept {
        (void)ptr;
        (void)place;
        // No-op: memory managed by pool
    }

protected:
    // =========================================================================
    // Protected constructor (only for derived classes)
    // =========================================================================

    /**
     * @brief Construct message base
     * @param type Message type identifier
     * @note Called by derived class constructors only
     * @note Cannot create MessageBase directly
     */
    MessageBase(MessageType::Type type)
        : mMsgType(type)
        , mResult(MessageResult::PENDING)
        , mProcessId(0)
        , mNeedsResponse(false)
        , mRefCount(1)
        , mCompletionSemaphore(nullptr)
        , mCompleted(false)
    {}

    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     * @note Actual cleanup handled by kernel infrastructure
     */
    virtual ~MessageBase() = default;

    // =========================================================================
    // Protected fields (accessible by derived message types)
    // =========================================================================

    MessageType::Type mMsgType; // Type identifier for message dispatch
    MessageResult mResult;      // Processing result status

private:
    // =========================================================================
    // Private fields (accessed directly by infrastructure via friend)
    // =========================================================================
    // 
    // NOTE: These fields are accessed directly by MessageManager ONLY.
    //       All other code uses public setters/getters.
    //
    // Public setters (anyone can call):
    // - setProcessId(): Set mProcessId (called by AppComm)
    // - setResult(): Set mResult (kernel after processing)
    //
    // MessageManager private access (via friend):
    // - mNeedsResponse: Set in initCompletionSemaphore() when timeoutMs > 0
    // - mRefCount: Reference counting
    // - mCompletionSemaphore: Semaphore lifecycle
    // - mCompleted: Completion flag
    // =========================================================================

    uint32_t mProcessId;                    // Process ID (sender or receiver)
    bool mNeedsResponse;                    // Response expected flag

    std::atomic<uint32_t> mRefCount;        // Reference counter (thread-safe)

    // Completion synchronization (for request-response pattern)
    void* mCompletionSemaphore;             // Opaque pointer to semaphore
    std::atomic<bool> mCompleted;           // Completion flag (thread-safe)

    // =========================================================================
    // Friend declaration (lifecycle management access)
    // =========================================================================

    friend class ::App::MessageManager;  // Only MessageManager accesses private fields

    // =========================================================================
    // Deleted operations (prevent copying)
    // =========================================================================

    MessageBase(const MessageBase&) = delete;
    MessageBase& operator=(const MessageBase&) = delete;
};

} // namespace SDK
