/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <cstdint>
#include <cstddef>


class VncBuffer
{
public:
    VncBuffer();
    explicit VncBuffer(size_t size);
    ~VncBuffer();

    VncBuffer(const VncBuffer &) = delete;
    VncBuffer &operator=(const VncBuffer &) = delete;

public:
    inline int fd() const
    {
        return mMemFd;
    }

    inline bool isValid() const
    {
        return (mBuffer != nullptr);
    }

    inline size_t capacity() const
    {
        return mSize;
    }

    inline size_t space() const
    {
        return mSize - (mHeadIndex - mTailIndex) - 1;
    }

    inline size_t size() const
    {
        return (mHeadIndex - mTailIndex);
    }

    inline bool empty() const
    {
        return (mTailIndex == mHeadIndex);
    }

    inline bool full() const
    {
        return (space() == 0);
    }

    inline void clear()
    {
        mTailIndex = mHeadIndex = 0;
    }

    inline void advanceTail(size_t amount)
    {
        if (__builtin_expect(((mHeadIndex - mTailIndex) < amount), false))
            mTailIndex = mHeadIndex;
        else
            mTailIndex += amount;

        // check if we've moved into the second map and if so reset both indexes
        // back into the first mapping
        if (__builtin_expect((mTailIndex >= mSize), false)) {
            mTailIndex -= mSize;
            mHeadIndex -= mSize;
        }
    }

    inline void advanceHead(size_t amount)
    {
        const size_t avail = space();
        if (__builtin_expect((avail < amount), false))
            mHeadIndex += avail;
        else
            mHeadIndex += amount;
    }


    template<class T = void>
    inline T* data()
    {
        return reinterpret_cast<T*>(mBuffer);
    }
    template<class T = void>
    inline const T* data() const
    {
        return reinterpret_cast<const T*>(mBuffer);
    }


    template<class T = void>
    inline T* head()
    {
        return reinterpret_cast<T*>(mBuffer + mHeadIndex);
    }
    template<class T = void>
    inline const T* head() const
    {
        return reinterpret_cast<const T*>(mBuffer + mHeadIndex);
    }


    template<class T = void>
    inline T* tail()
    {
        return reinterpret_cast<T*>(mBuffer + mTailIndex);
    }
    template<class T = void>
    inline const T* tail() const
    {
        return reinterpret_cast<const T*>(mBuffer + mTailIndex);
    }

private:
    int mMemFd;
    uint8_t *mBuffer;

    const size_t mSize;
    size_t mHeadIndex;
    size_t mTailIndex;

};
