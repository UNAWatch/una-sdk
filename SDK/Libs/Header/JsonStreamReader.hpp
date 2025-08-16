/**
 ******************************************************************************
 * @file    JsonStreamReader.hpp
 * @date    09-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   JSON streaming deserializer and helper for JSON parsing.
 ******************************************************************************
 */

#ifndef __JSON_STREAM_READER_HPP
#define __JSON_STREAM_READER_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

/**
 * @class JsonStreamReader
 * @brief A JSON deserializer that facilitates validating and extracting data
 *        from JSON buffers using coreJSON library.
 *
 * This class provides functionality to validate JSON buffers and retrieve
 * various types of data from them using query.
 *
 * Query format:  for details see description for JSON_Search() function.
 *
 * For example, if the provided buffer contains {"foo":"abc","bar":{"foo":"xyz"}},
 * then a search for 'foo' would output abc, and a search for 'bar.foo' would
 * output xyz.
 *
 * To get nested objects as string you can use method for retriveng strings
 *  `get(const char *query, const char *&outValue, size_t &outLen)`
 * 'bar' would output {"foo":"xyz"}
 *
 * If the provided buffer contains [123,456,{"foo":"abc","bar":[88,99]}], then
 * a search for '[1]' would output 456, '[2].foo' would output abc, and '[2].bar[0]'
 * would output 88.
 *
 */
class JsonStreamReader {
public:
    /**
     * @brief Default constructor. Initializes an empty reader.
     */
    JsonStreamReader();

    /**
     * @brief Constructor to initialize the reader with a JSON buffer.
     * @param data: Pointer to the JSON buffer.
     * @param len: Length of the JSON buffer.
     */
    JsonStreamReader(const char *data, size_t len);

    /**
     * @brief Load a new JSON buffer into the reader.
     * @param data: Pointer to the JSON buffer.
     * @param len: Length of the JSON buffer.
     */
    void loadBuffer(const char *data, size_t len);

    /**
     * @brief Validate the loaded JSON buffer.
     * @return `true` if the buffer is a valid JSON, `false` otherwise.
     */
    bool validate();

    /**
     * @brief Check if the JSON buffer is valid.
     * @return `true` if the buffer is valid, `false` otherwise.
     */
    bool isValid() const;

    /**
     * @brief Retrieve a `null` value for a given query.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue: `true` if the query contains `null`.
     * @return `true` if the query is found, `false` otherwise.
     */
    bool getNull(const char *query, bool &outValue) const;

    /**
     * @brief Retrieve a boolean value.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue The boolean value retrieved.
     * @return `true` if the query is found and the value is valid.
     */
    bool get(const char *query, bool &outValue) const;

    /**
     * @brief Retrieve an integer value.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue The value retrieved.
     * @return `true` if the query is found and the value is valid.
     */
    bool get(const char *query, int8_t &outValue) const;
    bool get(const char *query, uint8_t &outValue) const;
    bool get(const char *query, int16_t &outValue) const;
    bool get(const char *query, uint16_t &outValue) const;
    bool get(const char *query, int32_t &outValue) const;
    bool get(const char *query, uint32_t &outValue) const;
    bool get(const char *query, int64_t &outValue) const;
    bool get(const char *query, uint64_t &outValue) const;

    /**
     * @brief Retrieve an floating-point value.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue The value retrieved.
     * @return `true` if the query is found and the value is valid.
     */
    bool get(const char *query, float &outValue) const;
    bool get(const char *query, double &outValue) const;

    /**
     * @brief Retrieve a string value from the JSON buffer.
     * @note The string is not null-terminated, so its length is also provided.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue: Pointer to the start of the string value associated
     *             with the query.
     * @param[out] outLen: Length of the retrieved string value.
     * @return `true` if the query is found and the value is successfully retrieved,
     *         `false` otherwise.
     */
    bool get(const char *query, const char *&outValue, size_t &outLen) const;

    /**
     * @brief Retrieve a string value from the JSON buffer.
     * @note The string is not null-terminated, so its length is also provided.
     * @param query: The object keys and array indexes to search for.
     * @param[out] outValue: Reference to store the string value associated
     *             with the query.
     * @return `true` if the query is found and the value is successfully retrieved,
     *         `false` otherwise.
     */
    bool get(const char *query, std::string_view &outValue) const;

    /**
     * @brief Get the number of elements in an array specified by a query path.
     *
     * This method searches the JSON structure using the provided query 
     * string (e.g., "[2].bar")
     * and, if the located element is an array, counts the number of items in 
     * that array.
     *
     * For example, if the JSON data represents:
     *   [123,456,{"foo":"abc","bar":[88,99]}]
     * a query of "[2].bar" would locate the array [88,99] and return a length of 2.
     *
     * @param query: The query path string specifying the location of the array.
     * @param[out] outLength: The number of elements in the resulting array.
     * @return `true` if the query is successful and the array's length is determined, 
     *         `false` otherwise.
     */
    bool getArrayLength(const char *query, size_t &outLength) const;

private:
    const char *mData;  ///< Pointer to the JSON buffer.
    size_t mLen;        ///< Length of the JSON buffer.
    bool mIsValid;      ///< Validation status of the JSON buffer.
};

#endif /* __JSON_STREAM_READER_HPP */

