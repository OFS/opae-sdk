//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef _WIN32
#define _GNU_SOURCE
#include <sys/mman.h>
#include <pthread.h>
#else
#include <Windows.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"

// MAP_HUGE_SHIFT is defined since Linux 3.8
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

void mpfOsMemoryBarrier(void)
{
#ifndef _WIN32
    asm volatile ("mfence" : : : "memory");
#else
    MemoryBarrier();
#endif
}


fpga_result mpfOsPrepareMutex(
    mpf_os_mutex_handle* mutex
)
{
#ifndef _WIN32
    //
    // Linux mutex implemented with pthreads.
    //
    pthread_mutex_t* m = malloc(sizeof(pthread_mutex_t));
    if (NULL == m) return FPGA_NO_MEMORY;

    pthread_mutex_init(m, NULL);

    // Return an anonymous handle to the mutex
    *mutex = (mpf_os_mutex_handle)m;
    return FPGA_OK;
#else
    //
    // Windows mutex
    //
    HANDLE m = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m) return FPGA_NO_MEMORY;

    *mutex = (mpf_os_mutex_handle)m;
    return FPGA_OK;
#endif
}


fpga_result mpfOsReleaseMutex(
    mpf_os_mutex_handle mutex
)
{
#ifndef _WIN32
    pthread_mutex_t* m = (pthread_mutex_t*)mutex;
    pthread_mutex_destroy(m);
    free(m);
    return FPGA_OK;
#else
    HANDLE m = (HANDLE)mutex;
    CloseHandle(m);
    return FPGA_OK;
#endif
}


fpga_result mpfOsLockMutex(
    mpf_os_mutex_handle mutex
)
{
#ifndef _WIN32
    pthread_mutex_t* m = (pthread_mutex_t*)mutex;
    int s = pthread_mutex_lock(m);
    return (0 == s) ? FPGA_OK : FPGA_EXCEPTION;
#else
    HANDLE m = (HANDLE)mutex;
    DWORD s = WaitForSingleObject(m, INFINITE);
    return (WAIT_OBJECT_0 == s) ? FPGA_OK : FPGA_EXCEPTION;
#endif
}


fpga_result mpfOsUnlockMutex(
    mpf_os_mutex_handle mutex
)
{
#ifndef _WIN32
    pthread_mutex_t* m = (pthread_mutex_t*)mutex;
    int s = pthread_mutex_unlock(m);
    return (0 == s) ? FPGA_OK : FPGA_EXCEPTION;
#else
    HANDLE m = (HANDLE)mutex;
    return ReleaseMutex(m) ? FPGA_OK : FPGA_EXCEPTION;
#endif
}



// Round a length up to a multiple of the page size
static size_t roundUpToPages(
    size_t num_bytes,
    mpf_vtp_page_size page_size
)
{
    size_t page_bytes = mpfPageSizeEnumToBytes(page_size);
    return (num_bytes + page_bytes - 1) & ~(page_bytes - 1);
}


fpga_result mpfOsMapMemory(
    size_t num_bytes,
    mpf_vtp_page_size* page_size,
    void** buffer
)
{
    num_bytes = roundUpToPages(num_bytes, *page_size);
    size_t page_bytes = mpfPageSizeEnumToBytes(*page_size);

    if ((NULL == buffer) || (0 == num_bytes))
    {
        return FPGA_INVALID_PARAM;
    }

#ifndef _WIN32
    // POSIX

    // Compute the mmap flags
    const int base_flags = (MAP_PRIVATE | MAP_ANONYMOUS);
    int flags = base_flags;
    if (*page_size > MPF_VTP_PAGE_4KB)
    {
        flags |= MAP_HUGETLB;

        // Indicate the desired size.  The page size enumeration already uses
        // log2 encoding, which mmap expects.
        flags |= (*page_size << MAP_HUGE_SHIFT);
    }

    *buffer = mmap(NULL, num_bytes, (PROT_READ | PROT_WRITE), flags, 0, 0);

    if ((*buffer == MAP_FAILED) && (*page_size > MPF_VTP_PAGE_4KB))
    {
        // Failed to allocate at the requested page size.  Retry, but
        // don't demand huge pages.
        assert(MPF_VTP_PAGE_2MB == *page_size);

        // Indicate that the preallocated 2MB region wasn't used.  The
        // response doesn't mean that the underlying pages are necessarily
        // only 4KB, but they may be.
        *page_size = MPF_VTP_PAGE_4KB;

        // Without requesting 2MB pages, mmap won't guarantee alignment.
        // Pad the request so that we will handle alignment here.
        size_t alloc_bytes = num_bytes + page_bytes;
        void* base_buffer = mmap(NULL, alloc_bytes, (PROT_READ | PROT_WRITE), base_flags, 0, 0);
        if (base_buffer != MAP_FAILED)
        {
            // Align the buffer start to a page
            *buffer = (void*)(((size_t)base_buffer + page_bytes - 1) & ~(page_bytes - 1));

            // Release the unused portion at the start used for alignment
            if (*buffer != base_buffer)
            {
                size_t drop_bytes = (size_t)(*buffer - base_buffer);
                munmap(base_buffer, drop_bytes);
                alloc_bytes -= drop_bytes;
            }

            // Release any extra portion at the end of the buffer
            if (alloc_bytes > num_bytes)
            {
                munmap(*buffer + num_bytes, alloc_bytes - num_bytes);
            }
        }
    }

    if (*buffer == MAP_FAILED)
    {
        *buffer = NULL;
    }
#else
    // Windows

    // Implement me
    *buffer = NULL;
#endif

    if (*buffer == NULL) return FPGA_NO_MEMORY;

    return FPGA_OK;
}


fpga_result mpfOsUnmapMemory(
    void* buffer,
    size_t num_bytes
)
{
    if (NULL == buffer) return FPGA_INVALID_PARAM;

#ifndef _WIN32
    // POSIX

    if (munmap(buffer, num_bytes))
    {
        return FPGA_EXCEPTION;
    }
#else
    // Windows

    // Implement me
    return FPGA_EXCEPTION;
#endif

    return FPGA_OK;
}
