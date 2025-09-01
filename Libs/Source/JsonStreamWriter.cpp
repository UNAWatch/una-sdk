/**
 ******************************************************************************
 * @file    JsonStreamWriter.cpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   JSON streaming serializer.
 ******************************************************************************
 */

#include <cstdio>
#include <cstring>
#include <cassert>

#include "JsonStreamWriter.hpp"

namespace sdk
{

JsonStreamWriter::JsonStreamWriter(sdk::api::File *output) :
        out(output), outBuff(nullptr), outBuffSize(0), outbuffWritten(0),
        error(false), mContainerStackTop(0)
{
    // We don't create a root container automatically -
    // the user must call startMap() or startArray() to start the document.
}

JsonStreamWriter::JsonStreamWriter(char *output, size_t buffSize) :
        out(nullptr), outBuff(output), outBuffSize(buffSize), outbuffWritten(0),
        error(false), mContainerStackTop(0)
{
    // We don't create a root container automatically -
    // the user must call startMap() or startArray() to start the document.
}

JsonStreamWriter::JsonStreamWriter() :
        out(nullptr), outBuff(nullptr), outBuffSize(0), outbuffWritten(0),
        error(false), mContainerStackTop(0)
{
}

void JsonStreamWriter::setOutput(sdk::api::File *output)
{
    out = output;
    outBuff = nullptr;
    outBuffSize = 0;
    outbuffWritten = 0;
    error = false;
}

void JsonStreamWriter::setOutput(char *output, size_t buffSize)
{
    out = nullptr;
    outBuff = output;
    outBuffSize = buffSize;
    outbuffWritten = 0;
    error = false;
}

bool JsonStreamWriter::isError()
{
    return error;
}

void JsonStreamWriter::startMap(size_t size)
{
    if (Container *parent = currentContainer()) {
        if (parent->type == ContainerType::ARRAY) {
            if (!parent->first) {
                writeChar(',');
            } else {
                parent->first = false;
            }
        }
    }
    writeChar('{');
    Container cont;
    cont.type = ContainerType::OBJECT;
    cont.first = true;
    cont.expectingKey = true;        // the object initially expects a key
    pushContainer(cont);
}

void JsonStreamWriter::startMap(const char *key, size_t size)
{
    add(key);
    startMap(size);
}

void JsonStreamWriter::endMap()
{
    writeChar('}');
    popContainer();
    if (Container *parent = currentContainer()) {
        if (parent->type == ContainerType::OBJECT && !parent->expectingKey) {
            parent->expectingKey = true;
        }
    }
}

void JsonStreamWriter::startArray(size_t size)
{
    if (Container *parent = currentContainer()) {
        if (parent->type == ContainerType::ARRAY) {
            if (!parent->first) {
                writeChar(',');
            } else {
                parent->first = false;
            }
        } else if (parent->type == ContainerType::OBJECT
                && !parent->expectingKey) {
            // If the parent container is an object and the key is already output, just continue.
        }
    }
    writeChar('[');
    Container cont;
    cont.type = ContainerType::ARRAY;
    cont.first = true;
    cont.expectingKey = false;
    pushContainer(cont);
}

void JsonStreamWriter::startArray(const char *key, size_t size)
{
    add(key);
    startArray(size);
}

void JsonStreamWriter::endArray()
{
    writeChar(']');
    popContainer();
    if (Container *parent = currentContainer()) {
        if (parent->type == ContainerType::OBJECT && !parent->expectingKey) {
            parent->expectingKey = true;
        }
    }
}

void JsonStreamWriter::addNull(const char *key)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeData("null", 4);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, bool value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    if (value) {
        writeData("true", 4);
    } else {
        writeData("false", 5);
    }

    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, int8_t value)
{
    add(key, static_cast<int32_t>(value));
}

void JsonStreamWriter::add(const char *key, uint8_t value)
{
    add(key, static_cast<uint32_t>(value));
}

void JsonStreamWriter::add(const char *key, int16_t value)
{
    add(key, static_cast<int32_t>(value));
}

void JsonStreamWriter::add(const char *key, uint16_t value)
{
    add(key, static_cast<uint32_t>(value));
}

void JsonStreamWriter::add(const char *key, int32_t value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeInt(value);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, uint32_t value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeUint(value);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, int64_t value)
{
    add(key, static_cast<double>(value));
}

void JsonStreamWriter::add(const char *key, uint64_t value)
{
    add(key, static_cast<double>(value));
}

void JsonStreamWriter::add(const char *key, float value)
{
    add(key, static_cast<double>(value));
}

void JsonStreamWriter::add(const char *key, double value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeDouble(value);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, const char *value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeString(value);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *key, const std::string_view &value)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeString(value);
    curr->expectingKey = true;
}

void JsonStreamWriter::addNull()
{
    Container *curr = currentContainer();
    if (curr) {
        if (curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        } else if (curr->type == ContainerType::OBJECT && !curr->expectingKey) {
            // If we are in the object and the key is already displayed, we just continue.
        }
    }
    writeData("null", 4);
    if (curr && curr->type == ContainerType::OBJECT && !curr->expectingKey) {
        curr->expectingKey = true;
    }
}

void JsonStreamWriter::addHexString(const char *key, const uint8_t *value, size_t len)
{
    Container *curr = currentContainer();
    assert(curr && curr->type == ContainerType::OBJECT);
    if (!curr->first) {
        writeChar(',');
    } else {
        curr->first = false;
    }
    writeString(key);
    writeChar(':');
    writeHexString(value, len);
    curr->expectingKey = true;
}

void JsonStreamWriter::add(bool value)
{
    Container *curr = currentContainer();
    if (curr) {
        if (curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        } else if (curr->type == ContainerType::OBJECT && !curr->expectingKey) {
            // If we are in the object and the key is already displayed, we just continue.
        }
    }
    if (value) {
        writeData("true", 4);
    } else {
        writeData("false", 5);
    }
    if (curr && curr->type == ContainerType::OBJECT && !curr->expectingKey) {
        curr->expectingKey = true;
    }
}

void JsonStreamWriter::add(int8_t value)
{
    add(static_cast<int32_t>(value));
}

void JsonStreamWriter::add(uint8_t value)
{
    add(static_cast<uint32_t>(value));
}

void JsonStreamWriter::add(int16_t value)
{
    add(static_cast<int32_t>(value));
}

void JsonStreamWriter::add(uint16_t value)
{
    add(static_cast<uint32_t>(value));
}

void JsonStreamWriter::add(int32_t value)
{
    Container *curr = currentContainer();
    if (curr) {
        if (curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        } else if (curr->type == ContainerType::OBJECT && !curr->expectingKey) {
            // If we are in the object and the key is already displayed, we just continue.
        }
    }
    writeInt(value);
    if (curr && curr->type == ContainerType::OBJECT && !curr->expectingKey) {
        curr->expectingKey = true;
    }
}

void JsonStreamWriter::add(uint32_t value)
{
    Container *curr = currentContainer();
    if (curr) {
        if (curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        } else if (curr->type == ContainerType::OBJECT && !curr->expectingKey) {
            // If we are in the object and the key is already displayed, we just continue.
        }
    }
    writeUint(value);
    if (curr && curr->type == ContainerType::OBJECT && !curr->expectingKey) {
        curr->expectingKey = true;
    }
}

void JsonStreamWriter::add(int64_t value)
{
    add(static_cast<double>(value));
}

void JsonStreamWriter::add(uint64_t value)
{
    add(static_cast<double>(value));
}

void JsonStreamWriter::add(float value)
{
    add(static_cast<double>(value));
}

void JsonStreamWriter::add(double value)
{
    Container *curr = currentContainer();
    if (curr) {
        if (curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        } else if (curr->type == ContainerType::OBJECT && !curr->expectingKey) {
            // If the key is already displayed.
        }
    }
    writeDouble(value);
    if (curr && curr->type == ContainerType::OBJECT && !curr->expectingKey)
        curr->expectingKey = true;
}

void JsonStreamWriter::add(const char *value)
{
    Container *curr = currentContainer();
    if (curr && curr->type == ContainerType::OBJECT) {
        if (curr->expectingKey) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
            writeString(value);
            writeChar(':');
            curr->expectingKey = false;
        } else {
            writeString(value);
            curr->expectingKey = true;
        }
    } else {
        if (curr && curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        }
        writeString(value);
    }
}

void JsonStreamWriter::addHexString(const uint8_t *value, size_t len)
{
    Container *curr = currentContainer();
    if (curr && curr->type == ContainerType::OBJECT) {
        if (curr->expectingKey) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
            writeHexString(value, len);
            writeChar(':');
            curr->expectingKey = false;
        } else {
            writeHexString(value, len);
            curr->expectingKey = true;
        }
    } else {
        if (curr && curr->type == ContainerType::ARRAY) {
            if (!curr->first) {
                writeChar(',');
            } else {
                curr->first = false;
            }
        }
        writeHexString(value, len);
    }
}

void JsonStreamWriter::flush()
{
    flushOutput();
}

void JsonStreamWriter::pushContainer(const Container &cont)
{
    assert(mContainerStackTop < skContainerStackSize);
    mContainerStack[mContainerStackTop++] = cont;
}

JsonStreamWriter::Container JsonStreamWriter::popContainer()
{
    assert(mContainerStackTop > 0);
    return mContainerStack[--mContainerStackTop];
}

JsonStreamWriter::Container* JsonStreamWriter::currentContainer()
{
    return (mContainerStackTop > 0) ? &mContainerStack[mContainerStackTop - 1] :
                                      nullptr;
}

void JsonStreamWriter::writeChar(char c)
{
    writeData(&c, 1);
}

void JsonStreamWriter::writeInt(int32_t value)
{
    char buf[32] { };
    int len = snprintf(buf, sizeof(buf), "%ld", value);
    writeData(buf, len);
}

void JsonStreamWriter::writeUint(uint32_t value)
{
    char buf[32] { };
    int len = snprintf(buf, sizeof(buf), "%lu", value);
    writeData(buf, len);
}

void JsonStreamWriter::writeDouble(double value)
{
    char buf[32] { };
    int len = snprintf(buf, sizeof(buf), "%g", value);
    writeData(buf, len);
}

void JsonStreamWriter::writeString(const char *s)
{
    writeChar('\"');
    for (const char *p = s; *p; ++p) {
        if (*p == '\"' || *p == '\\') {
            writeChar('\\');
        }
        writeChar(*p);
    }
    writeChar('\"');
}

void JsonStreamWriter::writeString(std::string_view s)
{
    writeChar('\"');
    for (char c : s) {
        if (c == '\"' || c == '\\') {
            writeChar('\\');
        }
        writeChar(c);
    }
    writeChar('\"');
}

void JsonStreamWriter::writeHexString(const uint8_t *data, size_t len)
{
    writeChar('\"');
    for (size_t i = 0; i < len; ++i) {
        char hexBuff[3];
        snprintf(hexBuff, sizeof(hexBuff), "%02X", data[i]);
        writeData(hexBuff, 2);
    }
    writeChar('\"');
}

void JsonStreamWriter::writeData(const char *data, size_t len)
{
    if (error) {
        return;
    }

    size_t written = 0;
    if (out) {
        if (!out->write(data, len, written)) {
            error = true;
        }
    } else if (outBuff) {
        if (len + outbuffWritten < outBuffSize) {
            memcpy(&outBuff[outbuffWritten], data, len);
            outbuffWritten += len;
            outBuff[outbuffWritten] = '\0';
        } else {
            error = true;
        }
    }
}

void JsonStreamWriter::flushOutput()
{
    if (error) {
        return;
    }

    if (out) {
        out->flush();
    }
}

} /* namespace sdk */

