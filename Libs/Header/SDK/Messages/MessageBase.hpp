/**
 * @file    MessageBase.hpp
 * @date    11-12-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Base class for all messages exchanged between kernel and applications
 *
 * This file defines the base message class with reference counting,
 * completion synchronization, and pool management. All concrete message
 * types must inherit from MessageBase.
 *
 * Memory layout is stable across compilers (4-byte alignment).
 *
 * Shared between kernel and applications.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

#include "SDK/Messages/MessageTypes.hpp"

// Force 4-byte alignment for serialization compatibility
#pragma pack(push, 4)

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
    FAIL    = 2,        // Processing failed
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
 * Memory layout (4-byte aligned, 32 bytes total on 32-bit with vptr):
 * Offset | Size | Field
 * -------|------|------------------
 *   0    |  4   | vptr (virtual table pointer)
 *   4    |  4   | mMsgType
 *   8    |  1   | mResult
 *   9    |  1   | mReserved1
 *  10    |  1   | mNeedsResponse
 *  11    |  1   | mCompleted (atomic<bool>, takes 1 bytes)
 *  12    |  4   | mCompletionSemaphore (pointer)
 *  16    |  4   | mRefCount (atomic<uint32_t>, takes 4 bytes)
 *  20    |  4   | mProcessId
 *  24    |  4   | mReserved2
 *  28    |  4   | mReserved3
 *
 * Memory lifecycle:
 * 1. Allocated from pool (refCount = 1)
 * 2. Retained when placed in queue (refCount++)
 * 3. Released after processing (refCount--)
 * 4. Returned to pool when refCount reaches 0
 */
class alignas(4) MessageBase {
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
            case MessageResult::FAIL:    return "FAIL";
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
        , mReserved1(false)
        , mNeedsResponse(false)
        , mCompleted(false)
        , mCompletionSemaphore(nullptr)
        , mRefCount(1)
        , mProcessId(0)
        , mReserved2(0)
        , mReserved3(0)
    {}

    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     * @note Actual cleanup handled by kernel infrastructure
     */
    virtual ~MessageBase() = default;

    // =========================================================================
    // Memory layout fields (FIXED ORDER - DO NOT REORDER)
    // =========================================================================
    //
    // WARNING: Field order is critical for serialization!
    // Any changes to order or types will break binary compatibility.
    //
    // Layout guaranteed by:
    // 1. #pragma pack(4) - 4-byte alignment
    // 2. Fixed field order below
    // 3. static_assert size checks
    // =========================================================================

    // offset 0: vtable pointer (4 bytes on 32-bit systems)

    MessageType::Type mMsgType;     // 4 bytes: Type identifier for dispatch
    MessageResult mResult;          // 1 byte:  Processing result status

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
    // - mReserved1, mReserved2: Future expansion
    // =========================================================================

    bool mReserved1;                        // 1 bytes: Reserved for future use
    bool mNeedsResponse;                    // 1 byte:  Response expected flag

    // Completion synchronization (for request-response pattern)
    std::atomic<bool> mCompleted;           // 1 bytes: Completion flag (atomic takes)
    void* mCompletionSemaphore;             // 4 bytes: Opaque pointer to semaphore

    std::atomic<uint32_t> mRefCount;        // 4 bytes: Reference counter (thread-safe)

    uint32_t mProcessId;                    // 4 bytes: Process ID (sender/receiver)

    uint32_t mReserved2;                    // 4 bytes: Reserved for future use
    uint32_t mReserved3;                    // 4 bytes: Reserved for future use
    // The last field is 4 bytes and 4-byte aligned to prevent the compiler
    // from placing derived class fields into the base class's tail padding.

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

// Verify memory layout assumptions
// Note: Size includes vtable pointer (4 bytes on 32-bit systems)
static_assert(sizeof(MessageResult) == 1, "MessageResult must be 1 byte");
static_assert(sizeof(bool) == 1, "bool must be 1 byte");
static_assert(sizeof(std::atomic<uint32_t>) == 4, "atomic<uint32_t> must be 4 bytes");
static_assert(sizeof(std::atomic<bool>) == 1, "atomic<bool> must be 1 bytes");
static_assert(sizeof(MessageBase) == 32, "MessageBase size must be 32 bytes");

/**
 * @brief ID-only message without payload
 *
 * Lightweight message type used when only a MessageType identifier
 * is required and no additional payload data is needed.
 *
 * This class exists to support message allocation through
 * IAppComm::allocateMessage<T>(), which requires a default-constructible
 * message type. The actual MessageType is assigned explicitly after
 * allocation via setType().
 *
 * Typical usage:
 * - Allocate MessageID from the kernel message pool
 * - Set the desired MessageType using setType()
 * - Send the message through AppComm
 *
 * @note The default message type used in the constructor is a placeholder
 *       and must be overridden before sending the message.
 * @note This message does not carry any payload and is intended for
 *       fire-and-forget or simple command-style messages.
 */
struct MessageID : public SDK::MessageBase
{
    MessageID()
        : MessageBase(SDK::MessageType::COMMAND_APP_RUN) // Some default ID
    {}

    void setType(SDK::MessageType::Type type) noexcept
    {
        mMsgType = type;
    }
};

} // namespace SDK

#pragma pack(pop)
