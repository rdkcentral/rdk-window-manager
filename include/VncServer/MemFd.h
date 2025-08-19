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

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#if !defined(SYS_memfd_create)
#  if defined(__NR_memfd_create)
#    define SYS_memfd_create  __NR_memfd_create
#  elif defined(__aarch64__)
#    define SYS_memfd_create  279
#  elif defined(__arm__)
#    define SYS_memfd_create  385
#  else
#    error "memfd not supported on platform"
#  endif
#endif

#if !defined(MFD_CLOEXEC)
#  define MFD_CLOEXEC         0x0001U
#endif
#if !defined(MFD_ALLOW_SEALING)
#  define MFD_ALLOW_SEALING   0x0002U
#endif
#if !defined(MFD_HUGETLB)
#  define MFD_HUGETLB         0x0004U
#endif


namespace RdkWindowManager
{
    /*
        Syscall wrapper for the memfd_create function, needed because some
        versions of RDK are using old glibc that doesn't contain the memfd_create
        function, but all RDK kernels actually supports the call.

     */
    static inline int memfd_create(const char *name, unsigned int flags)
    {
        return syscall(SYS_memfd_create, name, flags);
    }
}

