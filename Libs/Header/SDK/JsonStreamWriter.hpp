/**
 ******************************************************************************
 * @file    JsonStreamWriter.hpp
 * @date    09-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   JSON streaming serializer.
 ******************************************************************************
 */

#ifndef __JSON_STREAM_WRITER_HPP
#define __JSON_STREAM_WRITER_HPP

#include <cstdint>
#include <array>
#include <string_view>

#include "API/FileSystem.hpp"

namespace sdk
{

/**
 * @class JsonStreamWriter
 * @brief A streaming JSON writer for serializing data into JSON format without
 *        creating large intermediate buffers.
 *
 * This class allows you to stream JSON output directly into a provided output
 * interface (`IFile`). It supports writing JSON objects and arrays, and provides
 * overloaded `add()` methods to serialize primitive data types and strings.
 */
class JsonStreamWriter {
public:

    /**
     * @brief Constructor with output target.
     * @param output: Output stream to write JSON to.
     */
    JsonStreamWriter(sdk::api::File *output);

    /**
     * @brief Constructor with output target.
     * @param output: Buffer to write JSON to.
     * @param buffSize: Output buffer size.
     */
    JsonStreamWriter(char *output, size_t buffSize);


    /**
     * @brief Default constructor without output target.
     * Must call `setOutput()` before writing.
     */
    JsonStreamWriter();

    /**
     * @brief Set the output stream for JSON output.
     * @param output: Target output stream implementing `IFile`.
     */
    void setOutput(sdk::api::File *output);

    /**
     * @brief Set the output stream for JSON output.
     * @param output: Buffer to write JSON to.
     * @param buffSize: Output buffer size.
     */
    void setOutput(char *output, size_t buffSize);

    /**
     * @brief Check if any error occurs during writing..
     * @retval 'true' - there was an error, JSON is not valid,
     *         'false' - no error, JSON is valid.
     */
    bool isError();

    /**
     * @brief Start a new JSON object (map).
     * @param size: Optional size hint for the number of keys. (Unused)
     */
    void startMap(size_t size = 0);

    /**
     * @brief Start a new JSON object (map) with a key.
     * @param key: Key name for the object.
     * @param size: Optional size hint for the number of keys. (Unused)
     */
    void startMap(const char *key, size_t size = 0);

    /**
     * @brief End the current JSON object (map).
     */
    void endMap();

    /**
     * @brief Start a new JSON array.
     * @param size: Optional size hint for the number of elements. (Unused)
     */
    void startArray(size_t size = 0);

    /**
     * @brief Start a new JSON array with a key.
     * @param key: Key name for the array.
     * @param size: Optional size hint for the number of elements. (Unused)
     */
    void startArray(const char *key, size_t size = 0);

    /**
     * @brief End the current JSON array.
     */
    void endArray();

    /**
     * @brief Add a null value with a key.
     * @param key: Key name.
     */
    void addNull(const char *key);

    /**
     * @brief Add a boolean value with a key.
     * @param key: Key name.
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
     * @brief Add a string value with a key.
     * @param key: Key name.
     * @param value: Null-terminated string.
     */
    void add(const char *key, const char *value);

    /**
     * @brief Add a string value with a key.
     * @param key: Key name.
     * @param value: Null-terminated string.
     */
    void add(const char *key, const std::string_view &value);

    /**
     * @brief Add a null value to the current container (array or object).
     */
    void addNull();

    /**
     * @brief Add a HEX string value with a key.
     * @param key: Key name.
     * @param value: byte array to encode.
     * @param len: The length of the byte array.
     */
    void addHexString(const char *key, const uint8_t *value, size_t len);

    /**
     * @brief Add a boolean value to the current container.
     * @param value: Boolean value.
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
     * @brief Add a string value to the current container.
     *        Can be used either as a key or as a value.
     * @param value: Null-terminated string.
     */
    void add(const char *value);

    /**
     * @brief Add a HEX string value to the current container.
     * @param value: byte array to encode.
     * @param len: The length of the byte array.
     */
    void addHexString(const uint8_t *value, size_t len);

    /**
     * @brief Flush the internal output buffer to the stream.
     */
    void flush();

    /**
     * @class ArrayScope
     * @brief RAII helper for automatic array closing.
     * Calls `endArray()` when going out of scope.
     */
    class ArrayScope {
    public:
        /**
         * @brief Constructor that starts a JSON array.
         * @param writer: Reference to the parent JsonStreamWriter.
         * @param size: Optional size hint. (Unused)
         */
        explicit ArrayScope(JsonStreamWriter &writer, size_t size = 0) :
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
        JsonStreamWriter &writer; ///< Reference to the parent writer.
    };

    /**
     * @class MapScope
     * @brief RAII helper for automatic map (object) closing.
     * Calls `endMap()` when going out of scope.
     */
    class MapScope {
    public:
        /**
         * @brief Constructor that starts a JSON map.
         * @param writer: Reference to the parent JsonStreamWriter.
         * @param size: Optional size hint. (Unused)
         */
        explicit MapScope(JsonStreamWriter &writer, size_t size = 0) :
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
        JsonStreamWriter &writer; ///< Reference to the parent writer.
    };

    /**
     * @class KeyedArrayScope
     * @brief RAII helper for key-value arrays.
     * Calls `endArray()` when going out of scope.
     */
    class KeyedArrayScope {
    public:
        /**
         * @brief Constructor that starts a JSON array.
         * @param writer: Reference to the parent JsonStreamWriter.
         * @param key: Key name of the array.
         * @param size: Optional size hint. (Unused)
         */
        explicit KeyedArrayScope(JsonStreamWriter &writer, const char *key,
                size_t size = 0) :
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
        JsonStreamWriter &writer;
    };

    /**
     * @class KeyedMapScope
     * @brief RAII helper for key-value nested maps.
     * Calls `endMap()` when going out of scope.
     */
    class KeyedMapScope {
    public:
        /**
         * @brief Constructor that starts a JSON map.
         * @param writer: Reference to the parent JsonStreamWriter.
         * @param key: Key name of the map.
         * @param size: Optional size hint. (Unused)
         */
        explicit KeyedMapScope(JsonStreamWriter &writer, const char *key,
                size_t size = 0) :
                writer(writer)
        {
            // Call a one-parameter add() to output the key,
            // after which the container is opened as an object.
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
        JsonStreamWriter &writer; ///< Reference to the parent writer.
    };

private:
    /// Output file interface.
    sdk::api::File *out;

    char *outBuff;
    size_t outBuffSize;
    size_t outbuffWritten;

    bool error;

    /**
     * @brief Enum representing the current context (object or array).
     */
    enum class ContainerType {
        ARRAY,   ///< Currently inside a JSON array
        OBJECT      ///< Currently inside a JSON object
    };

    // Structure for storing the state of an open container
    struct Container {
        ContainerType type;
        bool first; // Has at least one element of this container already been displayed
        bool expectingKey; // For an object: is a key expected (true)
    };

    /// Maximum container stack depth.
    static constexpr size_t skContainerStackSize = 16;

    /// Container stack for nested maps/arrays.
    std::array<Container, skContainerStackSize> mContainerStack { };

    /// Top index of the container stack.
    std::size_t mContainerStackTop = 0;

    /**
     * @brief Pushes a new container (array or map) onto the container stack.
     * @param cont: The container type to push (e.g., array or map).
     */
    void pushContainer(const Container &cont);

    /**
     * @brief Pops the most recent container off the container stack.
     * @return The popped container.
     */
    Container popContainer();

    /**
     * @brief Returns a pointer to the current container on top of the stack.
     * @return Pointer to the top container, or nullptr if the stack is empty.
     */
    Container* currentContainer();

    /**
     * @brief Writes a single character to the output stream.
     * @param c The character to write.
     */
    void writeChar(char c);

    /**
     * @brief Writes a signed 32-bit integer as a decimal string.
     * @param value: Integer to write.
     */
    void writeInt(int32_t value);

    /**
     * @brief Writes an unsigned 32-bit integer as a decimal string.
     * @param value: Unsigned integer to write.
     */
    void writeUint(uint32_t value);

    /**
     * @brief Writes a double-precision floating-point number.
     * @param value: Floating-point value to write.
     */
    void writeDouble(double value);

    /**
     * @brief Writes a string with JSON-compatible escaping and enclosing quotes.
     *
     * Escapes only `"` and `\\` characters. This method does not escape control
     * characters or Unicode, so it is suitable for simple JSON serialization but
     * not full compliance.
     *
     * @param s Null-terminated string to write.
     */
    void writeString(const char *s);

    /**
     * @brief Writes a string with JSON-compatible escaping and enclosing quotes.
     *
     * Escapes only `"` and `\\` characters. This method does not escape control
     * characters or Unicode, so it is suitable for simple JSON serialization but
     * not full compliance.
     *
     * @param s String to write.
     */
    void writeString(std::string_view s);

    /**
     * @brief Writes a HEX string with JSON-compatible enclosing quotes.
     * @param data: Pointer to the data buffer.
     * @param len: Number of bytes to write.
     */
    void writeHexString(const uint8_t *data, size_t len);

    /**
     * @brief Writes a block of raw data to the underlying output interface.
     * @param data: Pointer to the data buffer.
     * @param len: Number of bytes to write.
     */
    void writeData(const char *data, size_t len);

    /**
     * @brief Flushes the underlying output stream.
     */
    void flushOutput();
};


} /* namespace sdk */

#endif /* __JSON_STREAM_WRITER_HPP */
