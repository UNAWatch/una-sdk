/**
 ******************************************************************************
 * @file    Logger.c
 * @date    06-07-2023
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Logger - log to SEMIHOSTING, UART, USB_CDC, RTT, etc.
 ******************************************************************************
 *
 * COPYRIGHT(c) 2023 Droid-Technologies LLC
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of Droid-Technologies LLC nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 */

#include "SDK/UnaLogger/Logger.h"

#include <stdlib.h>
#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////
//// Private data
////////////////////////////////////////////////////////////////////////////////

static SDK::Interface::IMutex* mMutex;
static LoggerTimeFunc_t        mGetTime;
static LoggerPrint_t           mPrint;

////////////////////////////////////////////////////////////////////////////////
//// Private code
////////////////////////////////////////////////////////////////////////////////

static void Logger_vprintf(const char *format, va_list args)
{
    if (!mPrint) {
        return;
    }

    // Calculate the maximum length of the current message
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (len <= 0) {
        return;
    }

    char buffer[128];

    if ((len + 1) > static_cast<int>(sizeof(buffer))) {
        // The message does not fit in the buffer. It must be larger.
        char* buff = (char*) malloc(len + 1);
        if (buff) {
            vsnprintf(buff, len + 1, format, args);
            mPrint(buff);
            free(buff);
        }
    } else {
        // The message fit in the buffer.
        vsnprintf(buffer, sizeof(buffer), format, args);
        mPrint(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////
//// Public code
////////////////////////////////////////////////////////////////////////////////

void Logger_init(LoggerPrint_t print)
{
    mPrint = print;
}

void Logger_setMutex(SDK::Interface::IMutex* mutex)
{
    mMutex = mutex;
}

void Logger_setTimeFunc(LoggerTimeFunc_t pTimefn)
{
    mGetTime = pTimefn;
}

void Logger_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Logger_vprintf(format, args);
    va_end(args);
}

void Logger_printTimestamp()
{
    if (!mGetTime) {
        return;
    }

    uint32_t t = mGetTime();
    
    Logger_printf("%10lu ", t);
}

void Logger_printf_module(const char* level,
                          const char* module,
                          const char* func,
                          int         line,
                          const char* format,
                          ...)
{
    char buff[50];
    snprintf(buff, sizeof(buff), "%s%s::%d", module, func, line);

    Logger_printf("-%s- %-36s: ", level, buff);

    va_list args;
    va_start(args, format);
    Logger_vprintf(format, args);
    va_end(args);
}

void Logger_printDumpBytes(const void* pData, uint32_t len)
{
    uint32_t i = 0;

    uint8_t* p = (uint8_t*)pData;
    if (len > 0 && pData != NULL) {
        for (i = 0; i < len;) {
            if ((i & 0x0f) == 0) {
                Logger_printf("%s   ", (i == 0) ? "" : "\n");
            } else if ((i & 0x07) == 0 && i != 0) {
                    Logger_printf(" ");                          // 8 byte separator
            }
            Logger_printf(" %02X", p[i++]);
        }
        Logger_printf("\n");
    }
}

bool Logger_wait()
{
    if (mMutex) {
        mMutex->lock();
    }

    return true;
}

void Logger_release()
{
    if (mMutex) {
        mMutex->unLock();
    }
}
