/**
 ******************************************************************************
 * @file    StdLibWrappers.h
 * @date    21-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of wrappers for unsafe stdlib functions
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __STDLIB_WRAPPERS_H__
#define __STDLIB_WRAPPERS_H__

#include <time.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tm safe_gmtime(const time_t* time);                              // Safe gmtime
struct tm safe_localtime(const time_t* time);                           // Safe localtime
void      safe_strcpy(char* dest, const char* src, size_t destSize);    // Safe strncpy

#ifdef __cplusplus
}
#endif

#endif // __STDLIB_WRAPPERS_H__