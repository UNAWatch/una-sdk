/**
 ******************************************************************************
 * @file    Logger.hpp
 * @date    19-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Logger component for GUI.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __LOGGER_HPP
#define __LOGGER_HPP

#include "Interfaces/IMutex.hpp"

#include <stdio.h>

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

typedef uint32_t (*LoggerTimeFunc_t)(void);
typedef void     (*LoggerPrint_t)(const char* str);

void Logger_init(LoggerPrint_t print);
void Logger_setMutex(Interface::IMutex* mutex);
void Logger_setTimeFunc(LoggerTimeFunc_t pTimefn); 
void Logger_printf(const char* format, ...);
void Logger_printTimestamp();
void Logger_printf_module(const char* level, const char* module, const char* func, int line, const char* format, ...);
void Logger_printDumpBytes(const void* pData, uint32_t len);
bool Logger_wait();
void Logger_release();

#if !defined(LOG_FORMAT)
#define LOG_FORMAT              3
#endif

#if defined(LOG_USE_BUFFER)
#define LOG_BUFFER_SIZE         256
#endif

#ifndef LOG_MODULE_PRX
#define LOG_MODULE_PRX          "GUI::"
#endif

#ifndef LOG_MODULE_LEVEL
#define LOG_MODULE_LEVEL        LOG_LEVEL_DEBUG
#endif

/* Get file name from __FILE__ without full path */
#ifndef __FILE_NAME__
#define __FILE_NAME__       (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif


/* Log format example
 *
 * file: MyFile.c
 *
 * 0  #define LOG_MODULE_PRX  "MyPrefix::"
 * 1  #include "Logger.h"
 * 2
 * 3  void MyModule_function ()
 * 4  {
 * 5      LOG_INFO("some text\n");
 * 6  }
 *
 * */
#if (LOG_FORMAT == 1)
 // -I- MyModule       : some text
#define __MODULE_LEN__  ((strchr(__FUNCTION__, '_')) ? ((int)(strchr(__FUNCTION__, '_') - __FUNCTION__)) : 20)
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf("-%s- %-15.*s: " FMT, #LEVEL, __MODULE_LEN__, __FUNCTION__, ##__VA_ARGS__);
#elif (LOG_FORMAT == 2)
 // -I- MyFile.c       : some text
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf("-%s- %-15s: " FMT, #LEVEL, __FILE_NAME__, ##__VA_ARGS__);
#elif (LOG_FORMAT == 3)
 // -I- MyModule_function             : some text
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf("-%s- %-36s: " FMT, #LEVEL, __FUNCTION__, ##__VA_ARGS__);
#elif (LOG_FORMAT == 4)
 // -I-    5: MyFile.c       : some text
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf("-%s- %4d: %-15s: " FMT, #LEVEL, __LINE__, __FILE_NAME__, ##__VA_ARGS__);
#elif (LOG_FORMAT == 5)
 // -I-  MyPrefix::MyModule_function:5 some text
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf_module(#LEVEL, LOG_MODULE_PRX, __FUNCTION__, __LINE__, FMT, ##__VA_ARGS__);
#else
 // -I- some text
#define LOG_FORMAT_STR(LEVEL, FMT, ...) Logger_printf("-%s- " FMT, #LEVEL,  ##__VA_ARGS__);
#endif


#define LOG(LEVEL, FMT, ...)            do {                                                        \
                                            if (Logger_wait()) {                                    \
                                                Logger_printTimestamp();                            \
                                                LOG_FORMAT_STR(LEVEL, FMT, ##__VA_ARGS__)           \
                                                Logger_release();                                   \
                                            }                                                       \
                                        } while(0)

#define LOG_WP(...)                     do {                                \
                                            if (Logger_wait()) {            \
                                                Logger_printf(__VA_ARGS__); \
                                                Logger_release();           \
                                            }                               \
                                        } while(0)

#define LOG_DUMP(DATA,LEN)              do {                                        \
                                            if (Logger_wait()) {                    \
                                                Logger_printDumpBytes(DATA, LEN);   \
                                                Logger_release();                   \
                                            }                                       \
                                        } while(0)

#endif // (LOG_LEVEL > LOG_LEVEL_NO_LOG)

// User MACRO

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG && LOG_MODULE_LEVEL >= LOG_LEVEL_DEBUG)
#define LOG_DEBUG(FMT, ...)             LOG(D, FMT, ##__VA_ARGS__)
#define LOG_DEBUG_WP(...)               LOG_WP(__VA_ARGS__)
#define LOG_DEBUG_DUMP(DATA, LEN)       LOG_DUMP(DATA,LEN)
#define LOG_DEBUG_RAW(DATA, LEN)        Logger_raw(DATA,LEN);
#else
#define LOG_DEBUG(...)                  do { } while(0)
#define LOG_DEBUG_WP(...)               do { } while(0)
#define LOG_DEBUG_DUMP(...)             do { } while(0)
#define LOG_DEBUG_RAW(...)              do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_INFO && LOG_MODULE_LEVEL >= LOG_LEVEL_INFO)
#define LOG_INFO(FMT, ...)              LOG(I, FMT, ##__VA_ARGS__)
#define LOG_INFO_WP(...)                LOG_WP(__VA_ARGS__)
#define LOG_INFO_DUMP(DATA, LEN)        LOG_DUMP(DATA,LEN)
#else
#define LOG_INFO(...)                   do { } while(0)
#define LOG_INFO_WP(...)                do { } while(0)
#define LOG_INFO_DUMP(...)              do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARNING && LOG_MODULE_LEVEL >= LOG_LEVEL_WARNING)
#define LOG_WARNING(FMT, ...)           LOG(W, FMT, ##__VA_ARGS__)
#define LOG_WARNING_WP(...)             LOG_WP(__VA_ARGS__)
#define LOG_WARNING_DUMP(DATA, LEN)     LOG_DUMP(DATA,LEN)
#else
#define LOG_WARNING(...)                do { } while(0)
#define LOG_WARNING_WP(...)             do { } while(0)
#define LOG_WARNING_DUMP(...)           do { } while(0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_ERROR && LOG_MODULE_LEVEL >= LOG_LEVEL_ERROR)
#define LOG_ERROR(FMT, ...)             LOG(E, FMT, ##__VA_ARGS__)
#define LOG_ERROR_WP(...)               LOG_WP(__VA_ARGS__)
#define LOG_ERROR_DUMP(DATA, LEN)       LOG_DUMP(DATA,LEN)
#else
#define LOG_ERROR(...)                  do { } while(0)
#define LOG_ERROR_WP(...)               do { } while(0)
#define LOG_ERROR_DUMP(...)             do { } while(0)

#define LOG_WP(...)                     do { } while(0)
#define LOG_DUMP(...)                   do { } while(0)
#endif

#endif /* __GUI_LOGGER_HPP */
