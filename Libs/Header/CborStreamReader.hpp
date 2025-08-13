/**
 ******************************************************************************
 * @file    CborStreamReader.hpp
 * @date    09-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   CBOR streaming deserializer for parsing CBOR buffers using tinycbor.
 ******************************************************************************
 */

#ifndef __CBOR_STREAM_READER_HPP
#define __CBOR_STREAM_READER_HPP

extern "C" {
#include "cbor.h"
}

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// TODO: Add reading nested objects

/**
 * @class CborStreamReader
 * @brief A CBOR deserializer that facilitates validating and extracting data
 *        from a CBOR-encoded buffer.
 *
 * This class provides functionality to validate a CBOR buffer and retrieve
 * various types of values (boolean, integers, floating-point and text strings)
 * from a CBOR map. It uses the tinycbor library for decoding.
 */
class CborStreamReader {
public:
    /**
     * @brief Default constructor. Initializes an empty reader.
     */
    CborStreamReader()
        : mData(nullptr)
        , mLen(0)
        , mIsValid(false)
    {
    }

    /**
     * @brief Constructor with an initial CBOR buffer.
     * @param data Pointer to the CBOR data buffer.
     * @param len Length of the CBOR data.
     */
    CborStreamReader(const char *data, size_t len)
        : mData(data)
        , mLen(len)
        , mIsValid(false)
    {
    }

    /**
     * @brief Load a new CBOR buffer. This reinitializes the reader.
     * @param data Pointer to the CBOR data buffer.
     * @param len Length of the CBOR data.
     */
    void loadBuffer(const char *data, size_t len)
    {
        mData = data;
        mLen = len;
        mIsValid = false;
    }

    /**
     * @brief Validate and parse the loaded CBOR buffer.
     * @return `true` if the CBOR data is successfully parsed, `false` otherwise.
     */
    bool validate()
    {
        if (mData != nullptr && mLen > 0) {
            // initialize the parser for CBOR data; convert mData to uint8_t*
            CborError err = cbor_parser_init(reinterpret_cast<const uint8_t *>(mData), mLen, 0, &mParser, &mRoot);
            mIsValid = (err == CborNoError);
        } else {
            mIsValid = false;
        }
        return mIsValid;
    }

    /**
     * @brief Check if the CBOR buffer has been successfully parsed.
     * @return `true` if valid, `false` otherwise.
     */
    bool isValid() const { return mIsValid; }

    /**
     * @brief Retrieve a `null` value for the given key.
     *
     * For CBOR, null is represented as a simple value.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue Set to `true` if the key exists and its value is null.
     * @return `true` if the key is found (regardless of its null status), `false` otherwise.
     */
    bool getNull(const char *key, bool &outValue) const
    {
        if (!mIsValid) return false;

        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);
        if (err != CborNoError)
            return false;

        outValue = cbor_value_is_null(&element);
        return true;
    }

    /**
     * @brief Retrieve a boolean value.
     * @param key The CBOR key to search for.
     * @param[out] outValue The boolean value retrieved.
     * @return `true` if the key is found and the value is retrieved successfully.
     */
    bool get(const char *key, bool &outValue) const
    {
        if (!mIsValid) return false;

        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);
        if (err != CborNoError)
            return false;

        if (cbor_value_is_boolean(&element)) {
            bool b;
            err = cbor_value_get_boolean(&element, &b);
            if (err != CborNoError)
                return false;
            outValue = b;
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve an int8_t value.
     *
     * The value is extracted as a 32-bit integer and then cast to 8 bits.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The int8_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, int8_t &outValue) const
    {
        int32_t temp = 0;
        if (get(key, temp)) {
            outValue = static_cast<int8_t>(temp);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve a uint8_t value.
     *
     * The value is extracted as a 32-bit unsigned integer and then cast to 8 bits.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The uint8_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, uint8_t &outValue) const
    {
        uint32_t temp = 0;
        if (get(key, temp)) {
            outValue = static_cast<uint8_t>(temp);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve an int16_t value.
     *
     * The value is extracted as a 32-bit integer and then cast to 16 bits.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The int16_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, int16_t &outValue) const
    {
        int32_t temp = 0;
        if (get(key, temp)) {
            outValue = static_cast<int16_t>(temp);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve a uint16_t value.
     *
     * The value is extracted as a 32-bit unsigned integer and then cast to 16 bits.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The uint16_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, uint16_t &outValue) const
    {
        uint32_t temp = 0;
        if (get(key, temp)) {
            outValue = static_cast<uint16_t>(temp);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve an int32_t value.
     *
     * Extracts a 64-bit integer from the CBOR map and casts it to 32-bit.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The int32_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, int32_t &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError || !cbor_value_is_integer(&element))
            return false;
        int64_t temp;
        err = cbor_value_get_int64(&element, &temp);
        if (err != CborNoError)
            return false;
        outValue = static_cast<int32_t>(temp);
        return true;
    }

    /**
     * @brief Retrieve a uint32_t value.
     *
     * Extracts a 64-bit unsigned integer from the CBOR map and casts it to 32-bit.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The uint32_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, uint32_t &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError || !cbor_value_is_unsigned_integer(&element))
            return false;
        uint64_t temp;
        err = cbor_value_get_uint64(&element, &temp);
        if (err != CborNoError)
            return false;
        outValue = static_cast<uint32_t>(temp);
        return true;
    }

    /**
     * @brief Retrieve an int64_t value.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The int64_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, int64_t &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError || !cbor_value_is_integer(&element))
            return false;
        err = cbor_value_get_int64(&element, &outValue);
        return (err == CborNoError);
    }

    /**
     * @brief Retrieve a uint64_t value.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue The uint64_t value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, uint64_t &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError || !cbor_value_is_unsigned_integer(&element))
            return false;
        err = cbor_value_get_uint64(&element, &outValue);
        return (err == CborNoError);
    }

    /**
     * @brief Retrieve a float value.
     * @param key The CBOR key to search for.
     * @param[out] outValue The float value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, float &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError)
            return false;
        if (cbor_value_is_float(&element)) {
            err = cbor_value_get_float(&element, &outValue);
            return (err == CborNoError);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve a double value.
     * @param key The CBOR key to search for.
     * @param[out] outValue The double value retrieved.
     * @return `true` if successful, `false` otherwise.
     */
    bool get(const char *key, double &outValue) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError)
            return false;
        if (cbor_value_is_double(&element)) {
            err = cbor_value_get_double(&element, &outValue);
            return (err == CborNoError);
        }
        return false;
    }

    /**
     ******************************************************************************
     * @brief Retrieve a text string from the CBOR map.
     *
     * This method searches for the specified key in the CBOR map and retrieves the
     * associated value as a text string. The string is not necessarily null-terminated,
     * so its length is provided separately.
     *
     * @param key The CBOR key to search for.
     * @param[out] outValue Pointer to the beginning of the text string within the CBOR buffer.
     * @param[out] outLen Length of the text string.
     * @return `true` if the key is found and the value is successfully retrieved,
     *         `false` otherwise.
     ******************************************************************************
     */
    bool get(const char *key, const char *&outValue, size_t &outLen) const
    {
        if (!mIsValid) return false;
        CborValue element;
        CborError err = cbor_value_map_find_value(&mRoot, key, &element);

        if (err != CborNoError || !cbor_value_is_text_string(&element))
            return false;

        err = cbor_value_get_string_length(&element, &outLen);
        if (err != CborNoError)
            return false;

        const char *str = nullptr;
        CborValue temp = element;
        err = cbor_value_get_text_string_chunk(&temp, &str, &outLen, &temp);
        if (err != CborNoError || str == nullptr)
            return false;
        outValue = str;
        return true;
    }

private:
    const char *mData;      ///< Pointer to the CBOR data buffer.
    size_t mLen;            ///< Length of the CBOR data.
    bool mIsValid;          ///< Indicates whether the CBOR data was successfully parsed.
    CborParser mParser;     ///< CBOR parser structure (from tinycbor).
    CborValue mRoot;        ///< Root element of the parsed CBOR data.
};

#endif /* __CBOR_STREAM_READER_HPP */


#if 0
/**
 ******************************************************************************
 * @brief Search for a nested element using a query path string.
 *
 * This method interprets a query path (e.g., "root.bar.foo[1]" or "[2].bar[0]")
 * and traverses the CBOR structure accordingly. The query may specify keys (using
 * dot-notation) and array indices enclosed in square brackets.
 *
 * For example, given a CBOR representation of:
 *   {"foo":"abc","bar":{"foo":"xyz"}}
 * A query for "foo" returns "abc", "bar" returns {"foo":"xyz"}, and "bar.foo"
 * returns "xyz".
 *
 * Similarly, if the CBOR data represents:
 *   [123,456,{"foo":"abc","bar":[88,99]}]
 * Then a query for "[1]" returns 456, "[2].foo" returns "abc", and "[2].bar[0]"
 * returns 88.
 *
 * On success, the output parameter foundValue will reference the located CBOR
 * element. No null termination is done for text strings, so the pointer remains
 * valid as long as the original CBOR buffer is intact.
 *
 * @param query The query string specifying the search path.
 * @param[out] foundValue The CBOR value located by the query.
 * @return true if the element is successfully located, false otherwise.
 ******************************************************************************
 */
bool searchByPath(const char *query, CborValue &foundValue) const
{
    if (!mIsValid)
        return false;

    // Створюємо локальну копію запиту для зручності парсингу.
    std::string path(query);

    // Якщо запит починається з "root", пропускаємо цей токен.
    if (path.compare(0, 4, "root") == 0) {
        if (path.length() > 4 && (path[4] == '.' || path[4] == '['))
            path = path.substr(4);
        else
            path = "";
    }

    // Починаємо з кореневого контейнера.
    CborValue current = mRoot;

    // Обробляємо запит покроково.
    while (!path.empty()) {
        // Пропускаємо початкову крапку, якщо вона є.
        if (path[0] == '.')
            path = path.substr(1);

        // Якщо перший символ - '[': обробка індексу масиву.
        if (!path.empty() && path[0] == '[') {
            size_t closingPos = path.find(']');
            if (closingPos == std::string::npos)
                return false; // Помилка формату запиту.

            // Отримуємо числовий індекс із запиту.
            std::string indexStr = path.substr(1, closingPos - 1);
            int index = std::atoi(indexStr.c_str());

            // Переконуємося, що поточний елемент є масивом.
            if (!cbor_value_is_array(&current))
                return false;

            // Заходимо в контейнер масиву.
            CborValue element;
            CborError err = cbor_value_enter_container(&current, &element);
            if (err != CborNoError)
                return false;

            // Проходимо по елементам масиву до потрібного індексу.
            for (int i = 0; i < index; i++) {
                if (cbor_value_at_end(&element))
                    return false; // Індекс поза межами масиву.
                err = cbor_value_advance(&element);
                if (err != CborNoError)
                    return false;
            }
            // Призначаємо знайдений елемент як поточний.
            current = element;

            // Видаляємо оброблену частину запиту.
            path = path.substr(closingPos + 1);
        } else {
            // Обробляємо ключ об'єкта: шукаємо наступну крапку або відкриту квадратну дужку.
            size_t posDot = path.find('.');
            size_t posBracket = path.find('[');
            size_t tokenEnd = std::string::npos;

            if (posDot == std::string::npos && posBracket == std::string::npos)
                tokenEnd = path.length();
            else if (posDot == std::string::npos)
                tokenEnd = posBracket;
            else if (posBracket == std::string::npos)
                tokenEnd = posDot;
            else
                tokenEnd = std::min(posDot, posBracket);

            std::string token = path.substr(0, tokenEnd);
            if (token.empty())
                return false;

            // Переконуємося, що поточний елемент є мапою.
            if (!cbor_value_is_map(&current))
                return false;

            CborValue newVal;
            CborError err = cbor_value_map_find_value(&current, token.c_str(), &newVal);
            if (err != CborNoError)
                return false;

            current = newVal;

            // Видаляємо оброблений ключ із запиту.
            path = path.substr(tokenEnd);
        }
    }

    foundValue = current;
    return true;
}



/**
 * @brief Get the number of elements in an array specified by a query path.
 *
 * This method searches the CBOR structure using the provided query string (e.g., "[2].bar")
 * and, if the located element is an array, counts the number of items in that array.
 *
 * For example, if the CBOR data represents:
 *   [123,456,{"foo":"abc","bar":[88,99]}]
 * a query of "[2].bar" would locate the array [88,99] and return a length of 2.
 *
 * @param query The query path string specifying the location of the array.
 * @param[out] outLength The number of elements in the resulting array.
 * @return true if the query is successful and the array's length is determined, false otherwise.
 */
bool getArrayLength(const char *query, size_t &outLength) const
{
    if (!mIsValid)
        return false;

    // Шукаємо елемент за заданим шляхом.
    CborValue arr;
    if (!searchByPath(query, arr))
        return false;

    // Переконуємося, що знайдений елемент є масивом.
    if (!cbor_value_is_array(&arr))
        return false;

    // Входимо в контейнер масиву, щоб порахувати кількість елементів.
    CborValue it;
    CborError err = cbor_value_enter_container(&arr, &it);
    if (err != CborNoError)
        return false;

    outLength = 0;
    while (!cbor_value_at_end(&it)) {
        outLength++;
        err = cbor_value_advance(&it);
        if (err != CborNoError)
            return false;
    }
    cbor_value_leave_container(&arr, &it);
    return true;
}
#endif