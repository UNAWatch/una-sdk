/**
 ******************************************************************************
 * @file    FitBaseType.hpp
 * @brief   FIT base-type definitions for the native UNA FIT encoder.
 *
 * The FIT file format encodes every field value as one of a small set of
 * base types. Each base type has a one-byte identifier (whose high bit marks
 * whether the type is multi-byte / endian-sensitive), a fixed element size in
 * bytes, and a canonical "invalid" sentinel used when a declared field carries
 * no value. These are properties of the public FIT binary format.
 ******************************************************************************
 */

#ifndef __SDK_FIT_BASE_TYPE_HPP
#define __SDK_FIT_BASE_TYPE_HPP

#include <cstdint>

namespace SDK::Fit {

/**
 * @brief FIT base types, valued with their on-wire base-type identifier byte.
 *
 * The identifier byte is what appears in a definition message's field entry:
 * bit 7 is the "endian ability" flag (set for 2-, 4- and 8-byte types) and
 * bits 0-4 are the base-type number.
 */
enum class BaseType : uint8_t {
    Enum    = 0x00,
    SInt8   = 0x01,
    UInt8   = 0x02,
    SInt16  = 0x83,
    UInt16  = 0x84,
    SInt32  = 0x85,
    UInt32  = 0x86,
    String  = 0x07,
    Float32 = 0x88,
    Float64 = 0x89,
    UInt8z  = 0x0A,
    UInt16z = 0x8B,
    UInt32z = 0x8C,
    Byte    = 0x0D,
    SInt64  = 0x8E,
    UInt64  = 0x8F,
    UInt64z = 0x90,
};

/// On-wire base-type identifier byte for a definition message field entry.
constexpr uint8_t baseTypeId(BaseType t) { return static_cast<uint8_t>(t); }

/// True if the type is multi-byte and therefore endian-sensitive (bit 7 set).
constexpr bool baseTypeIsEndian(BaseType t) { return (baseTypeId(t) & 0x80u) != 0; }

/// Size in bytes of a single element of the given base type.
constexpr uint8_t baseTypeSize(BaseType t)
{
    switch (t) {
        case BaseType::Enum:    return 1;
        case BaseType::SInt8:   return 1;
        case BaseType::UInt8:   return 1;
        case BaseType::String:  return 1;
        case BaseType::UInt8z:  return 1;
        case BaseType::Byte:    return 1;
        case BaseType::SInt16:  return 2;
        case BaseType::UInt16:  return 2;
        case BaseType::UInt16z: return 2;
        case BaseType::SInt32:  return 4;
        case BaseType::UInt32:  return 4;
        case BaseType::Float32: return 4;
        case BaseType::UInt32z: return 4;
        case BaseType::Float64: return 8;
        case BaseType::SInt64:  return 8;
        case BaseType::UInt64:  return 8;
        case BaseType::UInt64z: return 8;
    }
    return 0;
}

/// Canonical "invalid" sentinel for the given base type (low `baseTypeSize`
/// bytes are significant).
constexpr uint64_t baseTypeInvalid(BaseType t)
{
    switch (t) {
        case BaseType::Enum:    return 0xFFu;
        case BaseType::SInt8:   return 0x7Fu;
        case BaseType::UInt8:   return 0xFFu;
        case BaseType::String:  return 0x00u;
        case BaseType::UInt8z:  return 0x00u;
        case BaseType::Byte:    return 0xFFu;
        case BaseType::SInt16:  return 0x7FFFu;
        case BaseType::UInt16:  return 0xFFFFu;
        case BaseType::UInt16z: return 0x0000u;
        case BaseType::SInt32:  return 0x7FFFFFFFu;
        case BaseType::UInt32:  return 0xFFFFFFFFu;
        case BaseType::Float32: return 0xFFFFFFFFu;
        case BaseType::UInt32z: return 0x00000000u;
        case BaseType::Float64: return 0xFFFFFFFFFFFFFFFFull;
        case BaseType::SInt64:  return 0x7FFFFFFFFFFFFFFFull;
        case BaseType::UInt64:  return 0xFFFFFFFFFFFFFFFFull;
        case BaseType::UInt64z: return 0x0000000000000000ull;
    }
    return 0;
}

}  // namespace SDK::Fit

#endif  // __SDK_FIT_BASE_TYPE_HPP
