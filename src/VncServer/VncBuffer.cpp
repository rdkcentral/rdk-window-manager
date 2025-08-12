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

#include "VncBuffer.h"
#include "MemFd.h"


#include <cstdio>
#include <cerrno>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "logger.h"
using namespace RdkWindowManager;

/*
    \class VncBuffer
    \brief Object that provides a low level ring buffer.

    This class provides a low level implementation of a ring buffer that uses
    some virtual memory tricks to provide a continuous memory range to the
    clients.

    Basically we mmap the same physical memory (created as an memfd) into two
    consecutive virtual memory address spaces, thereby providing clients with
    a continuous buffer regardless of wraps.  However it means we have to take
    control of the head and tail pointers to make sure they are always correctly
    point to the correct mapping.

 */


/*
    Small utility to sanitise the size of the buffer so that it is at least
    the minimum size and it's a multiple of a page.
 */
static inline size_t sanitiseBufferSize(size_t size)
{
    static const size_t pageSize = sysconf(_SC_PAGE_SIZE);

    size = std::max<size_t>(size, pageSize);
    return (size + (pageSize - 1)) & ~(pageSize - 1);
}


/* Constructs an invalid ring buffer. */
VncBuffer::VncBuffer()
    : mMemFd(-1)
    , mBuffer(nullptr)
    , mSize(0)
    , mHeadIndex(0)
    , mTailIndex(0)
{
}

/*
    Constructs a ring buffer of at least \a size bytes, the actual size of the
    buffer may be increased to meet minimum size and page alignment restrictions.

    Use isValid() to determine if the ring buffer was successfully created.
 */
VncBuffer::VncBuffer(size_t unalignedSize)
    : mMemFd(-1)
    , mBuffer(nullptr)
    , mSize(sanitiseBufferSize(unalignedSize))
    , mHeadIndex(0)
    , mTailIndex(0)
{
    char memName[32];
    sprintf(memName, "/vncbuffer-%08x", rand());

    // create a shared memory block of the correct buffer size
    int memFd = RdkWindowManager::memfd_create(memName, MFD_CLOEXEC);
    if (memFd < 0)
    {
        Logger::log(LogLevel::Error, "failed to create mmefd for buffer %d", errno);
        return;
    }

    if (ftruncate(memFd, mSize) != 0)
    {
        Logger::log(LogLevel::Error, "failed to resize mmefd for buffer %d", errno);
        close(memFd);
        return;
    }


    // now the tricky part, we need to reserve some virtual memory that is
    // twice as large as the buffer
    void *reserveMap = mmap(nullptr, (mSize * 2), PROT_NONE,
                            (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0);
    if (reserveMap == MAP_FAILED)
    {
        Logger::log(LogLevel::Error, "failed to reserve virtual space for the buffer %d", errno);
        close(memFd);
        return;
    }

    // now overlap the memfd buffer at the start of the reserve mapping created
    // above
    void *bufMap0 = mmap(reinterpret_cast<void*>(uintptr_t(reserveMap) + 0),
                         mSize, (PROT_READ | PROT_WRITE),
                         (MAP_SHARED | MAP_FIXED), memFd, 0);
    if (bufMap0 == MAP_FAILED)
    {
        Logger::log(LogLevel::Error, "failed to overlap memfd buffer 0 %d", errno);
        munmap(reserveMap, (mSize * 2));
        close(memFd);
        return;
    }

    // and map again right after the first mapping
    void *bufMap1 = mmap(reinterpret_cast<void*>(uintptr_t(reserveMap) + mSize),
                         mSize, (PROT_READ | PROT_WRITE),
                         (MAP_SHARED | MAP_FIXED), memFd, 0);
    if (bufMap1 == MAP_FAILED)
    {
        Logger::log(LogLevel::Error, "failed to overlap memfd buffer 1 %d", errno);
        munmap(reserveMap, (mSize * 2));
        close(memFd);
        return;
    }

    Logger::log(LogLevel::Information, "mapped ring buffer to 0x%p and 0x%p with size 0x%08zx", bufMap0, bufMap1, mSize);

    mMemFd = memFd;
    mBuffer = reinterpret_cast<uint8_t*>(bufMap0);
}

/* Destructor that cleans up the ring buffer. */
VncBuffer::~VncBuffer()
{
    // we unmap twice the size of the buffer as we mmap it twice
    if (mBuffer != nullptr)
        munmap(mBuffer, (mSize * 2));

    if ((mMemFd >= 0) && (close(mMemFd) != 0))
        Logger::log(LogLevel::Error, "failed to close shm %d", errno);

    mBuffer = nullptr;
    mMemFd = -1;
}
