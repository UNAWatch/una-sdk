/**
 ******************************************************************************
 * @file    CborStreamWriter.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   CBOR streaming serializer.
 ******************************************************************************
 */

#ifndef __CBOR_STREAM_WRITER_HPP
#define __CBOR_STREAM_WRITER_HPP

extern "C" {
#include "cbor.h"
}

#include <cstdint>
#include <array>

#include "IFileSystem.hpp"

/**
 * @class CborStreamWriter
 * @brief CBOR streaming serializer using IFile interface as output.
 *
 * This class allows you to stream CBOR output directly into a provided output
 * interface (`IFile`). It supports writing CBOR objects and arrays, and provides
 * overloaded `add()` methods to serialize primitive data types and strings.
 */
class CborStreamWriter {
public:
    /**
     * @brief Constructor with output file interface.
     * @param output Pointer to the file output interface.
     */
    CborStreamWriter(Interface::IFile *output);

    /**
     * @brief Default constructor.
     * Output must be set later using setOutput().
     */
    CborStreamWriter();

    /**
     * @brief Sets the output file interface.
     * @param output: Pointer to the file output interface.
     */
    void setOutput(Interface::IFile *output);

    /**
     * @brief Starts a CBOR map.
     * @param size: Number of elements, or CborIndefiniteLength.
     */
    void startMap(size_t size = CborIndefiniteLength);

    /**
     * @brief Starts a CBOR map with a key.
     * @param key: Name of the key.
     * @param size: Number of elements, or CborIndefiniteLength.
     */
    void startMap(const char *key, size_t size = CborIndefiniteLength);

    /**
     * @brief Ends the current CBOR map.
     */
    void endMap();

    /**
     * @brief Starts a CBOR array.
     * @param size: Number of elements, or CborIndefiniteLength.
     */
    void startArray(size_t size = CborIndefiniteLength);

    /**
     * @brief Starts a CBOR array with a key.
     * @param key: Name of the key.
     * @param size: Number of elements, or CborIndefiniteLength.
     */
    void startArray(const char *key, size_t size = CborIndefiniteLength);

    /**
     * @brief Ends the current CBOR array.
     */
    void endArray();

    /**
     * @brief Adds a null value with key.
     * @param key: Name of the key.
     */
    void addNull(const char *key);

    /**
     * @brief Adds a boolean value with key.
     * @param key: Name of the key.
     * @param value: Boolean value.
     */
    void add(const char *key, bool value);

    /**
     * @brief Add an integer value with a key.
     * @param key: Key name.
     * @param value: Integer value.
     */
    void add(const char *key, int8_t value);
    void add(const char *key, uint8_t value);
    void add(const char *key, int16_t value);
    void add(const char *key, uint16_t value);
    void add(const char *key, int32_t value);
    void add(const char *key, uint32_t value);
    void add(const char *key, int64_t value);
    void add(const char *key, uint64_t value);

    /**
     * @brief Add a floating-point value with a key.
     * @param key: Key name.
     * @param value: Float or double value.
     */
    void add(const char *key, float value);
    void add(const char *key, double value);

    /**
     * @brief Adds a string value with key.
     * @param key: Name of the key.
     * @param value: Null-terminated string.
     */
    void add(const char *key, const char *value);

    /**
     * @brief Adds a null value to the current container.
     */
    void addNull();

    /**
     * @brief Adds a boolean value to the current container.
     */
    void add(bool value);

    /**
     * @brief Add an integer value to the current container.
     * @param value: Integer value.
     */
    void add(int8_t value);
    void add(uint8_t value);
    void add(int16_t value);
    void add(uint16_t value);
    void add(int32_t value);
    void add(uint32_t value);
    void add(int64_t value);
    void add(uint64_t value);

    /**
     * @brief Add a floating-point value to the current container.
     * @param value: Float or double value.
     */
    void add(float value);
    void add(double value);

    /**
     * @brief Adds a string value.
     * @param value: Null-terminated string.
     */
    void add(const char *value);

    /**
     * @brief Flushes the output to the file.
     */
    void flush();

    /**
     * @class ArrayScope
     * @brief RAII helper for writing CBOR arrays.
     */
    class ArrayScope {
    public:
        /**
         * @brief Constructor.
         * @param writer: Reference to the CBOR writer.
         * @param size: Array size or CborIndefiniteLength.
         */
        explicit ArrayScope(CborStreamWriter &writer, size_t size =
                CborIndefiniteLength) :
                writer(writer)
        {
            writer.startArray(size);
        }

        /**
         * @brief Destructor.
         * Automatically ends the array.
         */
        ~ArrayScope()
        {
            writer.endArray();
        }
    private:
        CborStreamWriter &writer; ///< Reference to the parent writer.
    };

    /**
     * @class MapScope
     * @brief RAII helper for writing CBOR maps.
     */
    class MapScope {
    public:
        /**
         * @brief Constructor.
         * @param writer: Reference to the CBOR writer.
         * @param size: Map size or CborIndefiniteLength.
         */
        explicit MapScope(CborStreamWriter &writer, size_t size =
                CborIndefiniteLength) :
                writer(writer)
        {
            writer.startMap(size);
        }

        /**
         * @brief Destructor.
         * Automatically ends the map.
         */
        ~MapScope()
        {
            writer.endMap();
        }

    private:
        CborStreamWriter &writer; ///< Reference to the parent writer.
    };

    /**
     * @class KeyedArrayScope
     * @brief RAII helper for key-value arrays.
     */
    class KeyedArrayScope {
    public:
        /**
         * @brief Constructor.
         * @param writer: Reference to the CBOR writer.
         * @param key: Key name of the array.
         * @param size: Array size or CborIndefiniteLength.
         */
        explicit KeyedArrayScope(CborStreamWriter &writer, const char *key,
                size_t size = CborIndefiniteLength) :
                writer(writer)
        {
            writer.add(key);
            writer.startArray(size);
        }

        /**
         * @brief Destructor.
         * Automatically ends the array.
         */
        ~KeyedArrayScope()
        {
            writer.endArray();
        }

    private:
        CborStreamWriter &writer; ///< Reference to the parent writer.
    };

    /**
     * @class KeyedMapScope
     * @brief RAII helper for key-value nested maps.
     */
    class KeyedMapScope {
    public:
        /**
         * @brief Constructor.
         * @param writer: Reference to the CBOR writer.
         * @param key: Key name of the map.
         * @param size: Map size or CborIndefiniteLength.
         */
        explicit KeyedMapScope(CborStreamWriter &writer, const char *key,
                size_t size = CborIndefiniteLength) :
                writer(writer)
        {
            writer.add(key);
            writer.startMap(size);
        }

        /**
         * @brief Destructor.
         * Automatically ends the map.
         */
        ~KeyedMapScope()
        {
            writer.endMap();
        }

    private:
        CborStreamWriter &writer; ///< Reference to the parent writer.
    };

private:
    /// Output file interface.
    Interface::IFile *out;

    /// Root CBOR encoder.
    CborEncoder mRootEncoder;

    /// Maximum encoder stack depth.
    static constexpr size_t skEncoderStackSize = 16;

    /// Encoder stack for nested maps/arrays.
    std::array<CborEncoder, skEncoderStackSize> mEncoderStack;

    /// Top index of the encoder stack.
    std::size_t mEncoderStackTop = 0;

    /**
     * @brief Pushes a new encoder onto the stack.
     * @param encoder: The encoder to push.
     */
    void pushEncoder(const CborEncoder &encoder);

    /**
     * @brief Pops the top encoder from the stack.
     * @return Popped encoder.
     */
    CborEncoder popEncoder();

    /**
     * @brief Gets the current encoder.
     * @return Pointer to the current encoder.
     */
    CborEncoder* currentEncoder();

    /**
     * @brief libCBOR writer callback.
     * @param context: Writer context (this).
     * @param data: Data to write.
     * @param len: Length of data.
     * @param type: Append type.
     * @return CBOR error status.
     */
    static CborError WriteCallback(void *context, const void *data, size_t len,
            CborEncoderAppendType type);

    /**
     * @brief Flushes any buffered output.
     */
    void flushOutput();
};

#endif /* __CBOR_STREAM_WRITER_HPP */
