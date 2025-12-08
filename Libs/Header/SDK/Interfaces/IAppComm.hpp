/**
 * @file IAppComm.hpp
 * @date 02-12-2024
 * @author Kernel Communication Module
 * @brief Pure virtual interface for application-kernel communication
 *
 * This interface is provided by kernel to applications for message exchange.
 * Applications should only depend on this header and MessageTypes.hpp.
 * Actual implementation resides in kernel code.
 */

#pragma once

#include <cstdint>
#include <cstddef>


// Forward declarations
namespace SDK
{
    class MessageBase;
}


namespace SDK::Interface
{

/**
 * @brief Application communication interface
 *
 * This pure virtual interface provides communication methods between
 * application and kernel. Kernel creates concrete implementation and
 * passes it to application during initialization.
 *
 * Applications must NOT create instances directly - only use pointer
 * provided by kernel.
 */
class IAppComm {
public:

    // =========================================================================
    // Process identification
    // =========================================================================

    /**
     * @brief Get process ID assigned by kernel
     * @retval Unique process identifier
     * @note Process represents execution context (thread/task)
     * @note Application may consist of multiple processes (e.g. GUI + Service)
     */
    virtual uint32_t getProcessId() const = 0;

    // =========================================================================
    // Direction 1: Kernel -> Application
    // =========================================================================

    /**
     * @brief Receive message from kernel
     * @param msg Reference to message pointer (output parameter)
     * @param timeoutMs Timeout in milliseconds (0 for non-blocking)
     * @retval true Message received successfully
     * @retval false Timeout or error occurred
     *
     * @note Application must call releaseMessage() after processing
     * @note Reference counting managed automatically by kernel
     */
    virtual bool getMessage(MessageBase*& msg, uint32_t timeoutMs = 0xFFFFFFFF) = 0;

    /**
     * @brief Send response back to kernel (for request-response pattern)
     * @param msg Message with filled response fields
     *
     * @note Only valid for messages with needsResponse() == true
     * @note Sets completion flag and signals waiting kernel task
     * @note Does NOT release message - application must call releaseMessage()
     * @note If message does not need response, this call has no effect
     */
    virtual void sendResponse(MessageBase* msg) = 0;

    /**
     * @brief Release message object back to kernel pool
     * @param msg Message to release
     *
     * @note Decrements reference count
     * @note Returns message to pool when reference count reaches zero
     * @note After this call, message pointer becomes invalid
     * @note Application must NOT access message after release
     */
    virtual void releaseMessage(MessageBase* msg) = 0;

    // =========================================================================
    // Direction 2: Application -> Kernel
    // =========================================================================

    /**
     * @brief Send message to kernel
     * @param msg Message to send (must be allocated via allocateMessage)
     * @param timeoutMs Timeout in milliseconds (0 for non-blocking)
     * @retval true Message sent successfully and if timeoutMs > 0
     *         response received (check msg->getResult() for SUCCESS/ERROR)
     * @retval false Send failed (queue full or invalid message)
     *
     * @note Kernel automatically increments reference count when queuing
     * @note Message type determines if response is expected
     */
    virtual bool sendMessage(MessageBase* msg, uint32_t timeoutMs) = 0;

    /**
     * @brief Allocate typed message object from kernel pool
     * @retval Pointer to typed message on success
     * @retval nullptr if allocation failed
     *
     * @note Template wrapper for type-safe allocation
     * @note Calls placement new on allocated memory
     * @note Pool determined automatically by address during deallocation
     * @note Example: auto* msg = comm->allocateMessage<GpsRequestMsg>();
     */
    template<typename T>
    T* allocateMessage() {
        void* ptr = allocateMessage(sizeof(T));
        if (ptr) {
            // Placement new to construct typed object
            return new (ptr) T();
        }
        return nullptr;
    }

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IAppComm() = default;

    /**
     * @brief Allocate raw memory from kernel pool (internal use only)
     * @param size Size of message structure
     * @retval Pointer to raw memory on success
     * @retval nullptr if allocation failed
     * @note Memory is zeroed
     * @note Use template allocateMessage<T>() instead of calling this directly
     */
    virtual void* allocateMessage(size_t size) = 0;
};

} // namespace SDK::Interface
