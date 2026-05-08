/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2026 Garmin International, Inc.
// Licensed under the Flexible and Interoperable Data Transfer (FIT) Protocol License; you
// may not use this file except in compliance with the Flexible and Interoperable Data
// Transfer (FIT) Protocol License.
/////////////////////////////////////////////////////////////////////////////////////////////


#if !defined(FIT_CONFIG_H)
#define FIT_CONFIG_H


#if defined(__cplusplus)
   extern "C" {
#endif

// NOTE (una-sdk): Re-enabled to match the previous bundled SDK (21.171.00) and
// to avoid uint32_t/int32_t redefinition errors when building the TouchGFX
// simulator under MSVC, where <cstdint> is pulled in transitively and provides
// 'unsigned int'-based stdint typedefs that clash with the SDK's
// 'unsigned long'-based ones. If a future FIT SDK refresh overwrites this file,
// re-apply this define.
#define FIT_USE_STDINT_H // Define to use stdint.h types. By default size in bytes of integer types assumed to be char=1, short=2, long=4.

#define FIT_LOCAL_MESGS     16 // 1-16. Sets maximum number of local messages that can be decoded. Lower to minimize RAM requirements.
#define FIT_ARCH_ENDIAN     FIT_ARCH_ENDIAN_LITTLE   // Set to correct endian for build architecture.

#define FIT_CONVERT_CHECK_CRC // Define to check file crc.
#define FIT_CONVERT_CHECK_FILE_HDR_DATA_TYPE // Define to check file header for FIT data type.  Verifies file is FIT format before starting decode.
#define FIT_CONVERT_TIME_RECORD // Define to support time records (compressed timestamp).
//#define FIT_CONVERT_MULTI_THREAD // Define to support multiple conversion threads.
#define FIT_16BIT_MESG_LENGTH_SUPPORT
//#define FIT_STRUCT_OFFSET_USE_STDDEF_OFFSETOF // Define to use offsetof macro within FIT_STRUCT_OFFSET
//#define FIT_GET_MESG_DEF_BINARY_SEARCH // Define to use the binary serach version of Fit_GetMesgDef

#if defined(__cplusplus)
   }
#endif

#endif // !defined(FIT_CONFIG_H)
