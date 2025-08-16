/**
 ******************************************************************************
 * @file    JsonStreamReader.cpp
 * @date    09-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   JSON streaming deserializer and helper for JSON parsing.
 ******************************************************************************
 */

#include "JsonStreamReader.hpp"

extern "C" {
#include "core_json.h"
}

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdio.h>

JsonStreamReader::JsonStreamReader() :
        mData(nullptr), mLen(0), mIsValid(false)
{
}

JsonStreamReader::JsonStreamReader(const char *data, size_t len) :
        mData(data), mLen(len), mIsValid(false)
{
}

void JsonStreamReader::loadBuffer(const char *data, size_t len)
{
    mData = data;
    mLen = len;
    mIsValid = false;
}

bool JsonStreamReader::validate()
{
    if (mData != nullptr && mLen > 0) {
        mIsValid = (JSON_Validate(mData, mLen) == JSONSuccess);
    } else {
        mIsValid = false;
    }
    return mIsValid;
}

bool JsonStreamReader::isValid() const
{
    return mIsValid;
}

bool JsonStreamReader::getNull(const char *query, bool &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = (type == JSONNull);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, bool &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        if (type == JSONTrue) {
            outValue = true;
        } else if (type == JSONFalse) {
            outValue = false;
        } else if (type == JSONNumber) {
            int32_t temp = std::atoi(pValue);
            if (temp < 0 || temp > 1) {
                return false;
            }
            outValue = (temp > 0);
        }
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, int8_t &outValue) const
{
    int32_t temp = 0;
    if (get(query, temp)) {
        outValue = static_cast<int8_t>(temp);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, uint8_t &outValue) const
{
    uint32_t temp = 0;
    if (get(query, temp)) {
        outValue = static_cast<uint8_t>(temp);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, int16_t &outValue) const
{
    int32_t temp = 0;
    if (get(query, temp)) {
        outValue = static_cast<int16_t>(temp);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, uint16_t &outValue) const
{
    uint32_t temp = 0;
    if (get(query, temp)) {
        outValue = static_cast<uint16_t>(temp);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, int32_t &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = static_cast<int32_t>(std::atoi(pValue));
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, uint32_t &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = static_cast<uint32_t>(std::strtoul(pValue, nullptr, 10));
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, int64_t &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = static_cast<int64_t>(std::atoll(pValue));
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, uint64_t &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = static_cast<uint64_t>(std::strtoull(pValue, nullptr, 10));
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, float &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = static_cast<float>(std::atof(pValue));
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, double &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = std::atof(pValue);
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, const char *&outValue,
        size_t &outLen) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        outValue = pValue;
        outLen = valueLen;
        return true;
    }
    return false;
}

bool JsonStreamReader::get(const char *query, std::string_view &outValue) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type;
    if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                &valueLen, &type) == JSONSuccess)
    {
        std::string_view out {pValue, valueLen};
        outValue = out;
        return true;
    }
    return false;
}

bool JsonStreamReader::getArrayLength(const char *query,
        size_t &outLength) const
{
    const char *pValue = nullptr;
    size_t valueLen = 0;
    size_t queryLen = std::strlen(query);
    JSONTypes_t type;
    if (queryLen > 0) {
        if (JSON_SearchConst(mData, mLen, query, std::strlen(query), &pValue,
                    &valueLen, &type) != JSONSuccess)
        {
            return false;
        }
        if (type != JSONArray) {
            return false;
        }
    } else {
        // Look in root object
        pValue = mData;
        valueLen = mLen;
    }

    const char *pDummyValue = nullptr;
    size_t dummyLen = 0;
    size_t i = 0;
    JSONStatus_t status = JSONNotFound;
    char subquery[32] { };
    do {
        size_t ql = snprintf(subquery, sizeof(subquery), "[%u]", i++);
        status = JSON_SearchConst(pValue, valueLen, subquery, ql, &pDummyValue,
                &dummyLen, &type);
    } while (status == JSONSuccess);

    outLength = i - 1;
    return true;
}
