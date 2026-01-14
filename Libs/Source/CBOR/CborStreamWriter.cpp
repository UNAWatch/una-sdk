/**
 ******************************************************************************
 * @file    CborStreamWriter.cpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   CBOR streaming serializer.
 ******************************************************************************
 */

#include <cstring>
#include <cassert>

#include "CborStreamWriter.hpp"

CborStreamWriter::CborStreamWriter(Interface::IFile *output) :
        out(output), mEncoderStackTop(0)
{
    cbor_encoder_init_writer(&mRootEncoder, &CborStreamWriter::WriteCallback,
            this);
    pushEncoder(mRootEncoder);
}

CborStreamWriter::CborStreamWriter() :
        out(nullptr), mEncoderStackTop(0)
{
    cbor_encoder_init_writer(&mRootEncoder, &CborStreamWriter::WriteCallback,
            this);
    pushEncoder(mRootEncoder);
}

void CborStreamWriter::setOutput(Interface::IFile *output)
{
    out = output;
}

void CborStreamWriter::startMap(size_t size)
{
    CborEncoder map;
    cbor_encoder_create_map(currentEncoder(), &map, size);
    pushEncoder(map);
}

void CborStreamWriter::startMap(const char *key, size_t size)
{
    add(key);
    startMap(size);
}

void CborStreamWriter::endMap()
{
    CborEncoder map = popEncoder();
    cbor_encoder_close_container(currentEncoder(), &map);
}

void CborStreamWriter::startArray(size_t size)
{
    CborEncoder array;
    cbor_encoder_create_array(currentEncoder(), &array, size);
    pushEncoder(array);
}

void CborStreamWriter::startArray(const char *key, size_t size)
{
    add(key);
    startArray(size);
}

void CborStreamWriter::endArray()
{
    CborEncoder array = popEncoder();
    cbor_encoder_close_container(currentEncoder(), &array);
}

void CborStreamWriter::addNull(const char *key)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_null(currentEncoder());
}

void CborStreamWriter::add(const char *key, bool value)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_boolean(currentEncoder(), value);
}

void CborStreamWriter::add(const char *key, int8_t value)
{
    add(key, static_cast<int64_t>(value));
}

void CborStreamWriter::add(const char *key, uint8_t value)
{
    add(key, static_cast<uint64_t>(value));
}

void CborStreamWriter::add(const char *key, int16_t value)
{
    add(key, static_cast<int64_t>(value));
}

void CborStreamWriter::add(const char *key, uint16_t value)
{
    add(key, static_cast<uint64_t>(value));
}

void CborStreamWriter::add(const char *key, int32_t value)
{
    add(key, static_cast<int64_t>(value));
}

void CborStreamWriter::add(const char *key, uint32_t value)
{
    add(key, static_cast<uint64_t>(value));
}

void CborStreamWriter::add(const char *key, int64_t value)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_int(currentEncoder(), value);
}

void CborStreamWriter::add(const char *key, uint64_t value)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_uint(currentEncoder(), value);
}

void CborStreamWriter::add(const char *key, float value)
{
    add(key, static_cast<double>(value));
}

void CborStreamWriter::add(const char *key, double value)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_double(currentEncoder(), value);
}

void CborStreamWriter::add(const char *key, const char *value)
{
    cbor_encode_text_stringz(currentEncoder(), key);
    cbor_encode_text_stringz(currentEncoder(), value);
}

void CborStreamWriter::addNull()
{
    cbor_encode_null(currentEncoder());
}

void CborStreamWriter::add(bool value)
{
    cbor_encode_boolean(currentEncoder(), value);
}

void CborStreamWriter::add(int8_t value)
{
    add(static_cast<int64_t>(value));
}

void CborStreamWriter::add(uint8_t value)
{
    add(static_cast<uint64_t>(value));
}

void CborStreamWriter::add(int16_t value)
{
    add(static_cast<int64_t>(value));
}

void CborStreamWriter::add(uint16_t value)
{
    add(static_cast<uint64_t>(value));
}

void CborStreamWriter::add(int32_t value)
{
    add(static_cast<int64_t>(value));
}

void CborStreamWriter::add(uint32_t value)
{
    add(static_cast<uint64_t>(value));
}

void CborStreamWriter::add(int64_t value)
{
    cbor_encode_int(currentEncoder(), value);
}

void CborStreamWriter::add(uint64_t value)
{
    cbor_encode_uint(currentEncoder(), value);
}

void CborStreamWriter::add(float value)
{
    add(static_cast<double>(value));
}

void CborStreamWriter::add(double value)
{
    cbor_encode_double(currentEncoder(), value);
}

void CborStreamWriter::add(const char *value)
{
    cbor_encode_text_stringz(currentEncoder(), value);
}

void CborStreamWriter::flush()
{
    flushOutput();
}

void CborStreamWriter::pushEncoder(const CborEncoder &encoder)
{
    assert(mEncoderStackTop < skEncoderStackSize);
    mEncoderStack[mEncoderStackTop++] = encoder;
}

CborEncoder CborStreamWriter::popEncoder()
{
    assert(mEncoderStackTop > 0);
    return mEncoderStack[--mEncoderStackTop];
}

CborEncoder* CborStreamWriter::currentEncoder()
{
    assert(mEncoderStackTop > 0);
    return &mEncoderStack[mEncoderStackTop - 1];
}

// Output a block of data
CborError CborStreamWriter::WriteCallback(void *context, const void *data,
        size_t len, CborEncoderAppendType type)
{
    (void)type;
    CborStreamWriter *self = static_cast<CborStreamWriter*>(context);
    size_t written = 0;
    self->out->write(static_cast<const char*>(data), len, written);
    return (written == len) ? CborNoError : CborErrorIO;
}

// Function to flush the buffer
void CborStreamWriter::flushOutput()
{
    out->flush();
}

