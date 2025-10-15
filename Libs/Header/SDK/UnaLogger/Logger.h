/**
 ******************************************************************************
 * @file    Logger.h
 * @date    04-10-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Logger component for application.
 ******************************************************************************
 *
 * Usage example:
 *
 * file: MyFile.cpp
 * @code{.cpp}
 *
 * #define LOG_MODULE_PRX    "MyPrefix"
 * #define LOG_MODULE_LEVEL  LOG_LEVEL_DEBUG
 * #include "SDK/UnaLogger/Logger.h"
 *
 * void MyModule::function()
 * {
 *     LOG_INFO("some text\n");
 *
 *     uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
 *     LOG_DEBUG_DUMP(data, sizeof(data));
 * }
 *
 * @endcode
 *
 ******************************************************************************
 */

#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "SDK/Interfaces/ILogger.hpp"

// Log level
#define LOG_LEVEL_NO_LOG        0
#define LOG_LEVEL_ERROR         1
#define LOG_LEVEL_WARNING       2
#define LOG_LEVEL_INFO          3
#define LOG_LEVEL_DEBUG         4

#if !defined(LOG_LEVEL)
#define LOG_LEVEL               4
#endif

#if (LOG_LEVEL > LOG_LEVEL_NO_LOG)

// Get file name from __FILE__ without full path
#ifndef __FILENAME__
#if defined(_WIN32) || defined(_WIN64)
    #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#endif

// Function name macro
#ifndef __FUNCTION_NAME__
#define __FUNCTION_NAME__       __func__
#endif

#ifndef LOG_MODULE_PRX
#define LOG_MODULE_PRX          __FILENAME__
#endif

#ifndef LOG_MODULE_LEVEL
#define LOG_MODULE_LEVEL        LOG_LEVEL_DEBUG
#endif

/**
 * @brief   Initialize logger with kernel logging interface.
 *
 * This function initializes the logger by setting the internal static
 * pointer to the provided ILogger interface instance. It should be called
 * once during application initialization.
 *
 * @param[in]   ilogger     Reference to the ILogger interface instance
 *
 * @note This function does nothing if the logger is already initialized.
*/
void Logger_init(SDK::Interface::ILogger& ilogger);

/**
 * @brief   Output formatted log message with metadata.
 *
 * This function formats and outputs a log message with metadata including
 * module name, function name, and line number. The message is passed to
 * the kernel's logging interface.
 *
 * @param[in]   level       Log level string (e.g., "D", "I", "W", "E")
 * @param[in]   module      Module name or identifier
 * @param[in]   module_sep  Separator string between module and function (e.g., "::")
 * @param[in]   func        Function name (typically __FUNCTION_NAME__)
 * @param[in]   line        Line number (typically __LINE__)
 * @param[in]   fmt         Format string (printf-style)
 * @param[in]   ...         Variable arguments for format string
 *
 * @note The formatted output follows the pattern: -LEVEL- module::func::line: message
 */
void Logger_message(const char *level,
                    const char *module,
                    const char *func,
                    int line,
                    const char *fmt, ...);

/**
 * @brief   Print hexadecimal dump with metadata.
 *
 * This function outputs a formatted hexadecimal dump of binary data with
 * metadata. The output includes offset, hex values in rows of 16 bytes
 * with space separator at 8th byte.
 *
 * @param[in]   level       Log level string (e.g., "D", "I", "W", "E")
 * @param[in]   module      Module name or identifier
 * @param[in]   module_sep  Separator string between module and function (e.g., "::")
 * @param[in]   func        Function name (typically __FUNCTION_NAME__)
 * @param[in]   line        Line number (typically __LINE__)
 * @param[in]   pData       Pointer to data to dump
 * @param[in]   len         Length of data in bytes
 *
 * @note If pData is nullptr or len is 0, the function returns without output.
 * @note Output format: XXXX:  HH HH HH HH HH HH HH HH  HH HH HH HH HH HH HH HH
 */
void Logger_hexdump(const char *level,
                    const char *module,
                    const char *func,
                    int line,
                    const void *pData,
                    int len);

/**
 * @brief Generic log macro that outputs a formatted message with metadata.
 * @param LEVEL Log level identifier (D, I, W, E)
 * @param FMT Format string (printf-style)
 * @param ... Variable arguments for format string
 */
#define LOG(LEVEL, FMT, ...) \
        Logger_message(#LEVEL, LOG_MODULE_PRX, __FUNCTION_NAME__, __LINE__, FMT, ##__VA_ARGS__)

/**
 * @brief Generic hexdump macro that outputs binary data with metadata.
 * @param LEVEL Log level identifier (D, I, W, E)
 * @param DATA Pointer to data to dump
 * @param LEN Length of data in bytes
 */
#define LOG_DUMP(LEVEL, DATA, LEN)  \
        Logger_hexdump(#LEVEL, LOG_MODULE_PRX, __FUNCTION_NAME__, __LINE__, DATA, LEN)


#else
#define Logger_message(...)             do { } while(0)
#define Logger_hexdump(...)             do { } while(0)

#define LOG(...)                        do { } while(0)
#define LOG_DUMP(...)                   do { } while(0)

#endif // (LOG_LEVEL > LOG_LEVEL_NO_LOG)

// User MACRO

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG && LOG_MODULE_LEVEL >= LOG_LEVEL_DEBUG)
/**
 * @brief Debug level logging macro.
 * @param FMT Format string (printf-style)
 * @param ... Variable arguments for format string
 */
#define LOG_DEBUG(FMT, ...)             LOG(D, FMT, ##__VA_ARGS__)

/**
 * @brief Debug level hexdump macro.
 * @param DATA Pointer to data to dump
 * @param LEN Length of data in bytes
 */
#define LOG_DEBUG_DUMP(DATA, LEN)       LOG_DUMP(D, DATA, LEN)
#else
#define LOG_DEBUG(...)                  do { } while(0)
#define LOG_DEBUG_DUMP(...)             do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_INFO && LOG_MODULE_LEVEL >= LOG_LEVEL_INFO)
/**
 * @brief Info level logging macro.
 * @param FMT Format string (printf-style)
 * @param ... Variable arguments for format string
 */
#define LOG_INFO(FMT, ...)              LOG(I, FMT, ##__VA_ARGS__)

/**
 * @brief Info level hexdump macro.
 * @param DATA Pointer to data to dump
 * @param LEN Length of data in bytes
 */
#define LOG_INFO_DUMP(DATA, LEN)        LOG_DUMP(I, DATA, LEN)
#else
#define LOG_INFO(...)                   do { } while(0)
#define LOG_INFO_DUMP(...)              do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARNING && LOG_MODULE_LEVEL >= LOG_LEVEL_WARNING)
/**
 * @brief Warning level logging macro.
 * @param FMT Format string (printf-style)
 * @param ... Variable arguments for format string
 */
#define LOG_WARNING(FMT, ...)           LOG(W, FMT, ##__VA_ARGS__)

/**
 * @brief Warning level hexdump macro.
 * @param DATA Pointer to data to dump
 * @param LEN Length of data in bytes
 */
#define LOG_WARNING_DUMP(DATA, LEN)     LOG_DUMP(W, DATA, LEN)
#else
#define LOG_WARNING(...)                do { } while(0)
#define LOG_WARNING_DUMP(...)           do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_ERROR && LOG_MODULE_LEVEL >= LOG_LEVEL_ERROR)
/**
 * @brief Error level logging macro.
 * @param FMT Format string (printf-style)
 * @param ... Variable arguments for format string
 */
#define LOG_ERROR(FMT, ...)             LOG(E, FMT, ##__VA_ARGS__)

/**
 * @brief Error level hexdump macro.
 * @param DATA Pointer to data to dump
 * @param LEN Length of data in bytes
 */
#define LOG_ERROR_DUMP(DATA, LEN)       LOG_DUMP(E, DATA, LEN)
#else
#define LOG_ERROR(...)                  do { } while(0)
#define LOG_ERROR_DUMP(...)             do { } while(0)
#endif

#endif // __LOGGER_H
