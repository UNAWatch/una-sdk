/**
 ******************************************************************************
 * @file    RingBuffer.hpp
 * @date    22-August-2019
 * @brief   Header file for RingBuffer class.
 ******************************************************************************
 *
 * COPYRIGHT(c) 2019 Droid-Technologies LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Droid-Technologies LLC nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#pragma once

#include "Porting.h"

#include <assert.h>

namespace SDK::Tools {
/**
 * @brief This template class implements ring buffer for data with type T.
 * @note  None.
 * @bug   None.
 */
template<typename T>
class RingBuffer {
public:
    /**
     * @brief  Class constructor.
     * @param  bufferSize: uint16_t value of ring buffer size
     * @param  name: pointer to the buffer name
     * @param  enableOverload: enable buffer overload
     */
    RingBuffer(uint16_t bufferSize, const char* name, bool enableOverload = false)
    {
        mOverload = false;

        mBuffer = new T[bufferSize];

        ASSERT(mBuffer != nullptr);

        mBufferSize = bufferSize;
        mGetIndex   = 0;
        mPutIndex   = 0;
        mDataLength = 0;

        mName = (name == nullptr) ? "noname" : name;

        mEnableOverload = enableOverload;
    }

    /**
     * @brief   Class destructor.
     */
    virtual ~RingBuffer()
    {
        if (mBuffer != nullptr) {
            delete [] mBuffer;
        }
    }

    /**
     * @brief   Put new element to ring buffer.
     * @param   element:  new element.
     */
    void push(const T& element)
    {
        mBuffer[mPutIndex] = element;
        if (++mPutIndex == mBufferSize) {
            mPutIndex = 0;
        }

        if (++mDataLength > mBufferSize) {
            mDataLength = mBufferSize;

            if (!mEnableOverload) {
                //LOG_ERROR("[%s] overloaded\n", mName);
                assert(false);
            }

            mOverload = true;

            if (++mGetIndex >= mBufferSize) {
                mGetIndex = 0;
            }
        }
    }

    /**
     * @brief  Get first element from ring buffer.
     * @param  data: the created variable where to save element
     * @retval true: in the case of success.
     */
    bool pop(T &data)
    {
        if (mDataLength == 0) {
            return false;
        }

        data = mBuffer[mGetIndex];
        if (++mGetIndex == mBufferSize) {
            mGetIndex = 0;
        }

        --mDataLength;

        return true;
    }

    /**
     * @brief  Get first element from ring buffer.
     * @param  data: the created variable where to save element
     * @retval true: in the case of success.
     */
    bool peek(T &data)
    {
        if (mDataLength == 0) {
            return false;
        }

        data = mBuffer[mGetIndex];

        return true;
    }

    /**
     * @brief   Get a length of received elements in ring buffer.
     * @retval  uint16_t: length of received elements.
     */
    uint16_t getAvailForRead()
    {
        return mDataLength;
    }

    /**
     * @brief   Get a length of received elements in ring buffer.
     * @retval  uint16_t: length of received elements.
     */
    uint16_t getAvailForWrite()
    {
        return mBufferSize - mDataLength;
    }

    /**
     * @brief   Clear the ring buffer.
     */
    void clear()
    {
        mGetIndex = mPutIndex = mDataLength = 0;
        mOverload = false;
    }

    bool getOverload()
    {
        bool res = mOverload;

        mOverload = false;

        return res;
    }

private:
    T*       mBuffer;
    uint16_t mBufferSize;
    uint16_t mPutIndex;
    uint16_t mGetIndex;
    uint16_t mDataLength;
    bool     mOverload;

    const char* mName;

    bool mEnableOverload;
};
}

/******************* (C) COPYRIGHT Droid-Technologies LLC *********************/
