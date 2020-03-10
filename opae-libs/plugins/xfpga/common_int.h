// Copyright(c) 2017-2020, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __FPGA_COMMON_INT_H__
#define __FPGA_COMMON_INT_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>   /* bool type */
#include <malloc.h>    /* malloc */
#include <stdlib.h>    /* exit */
#include <stdio.h>     /* printf */
#include <string.h>    /* memcpy */
#include <unistd.h>    /* getpid */
#include <sys/types.h> /* pid_t */
#include <sys/ioctl.h> /* ioctl */
#include <sys/mman.h>  /* mmap & munmap */
#include <sys/time.h>  /* struct timeval */
#include <pthread.h>
#undef  _GNU_SOURCE

#include <opae/log.h>
#include "types_int.h"
#include "sysfs_int.h"
#include "wsid_list_int.h"
#include "token_list_int.h"
#include "mmap_int.h"
#include "props.h"

#define GCC_VERSION (__GNUC__       * 10000 \
		   + __GNUC_MINOR__ * 100 \
		   + __GNUC_PATCHLEVEL__)

#define KB 1024
#define MB (1024 * KB)
#define GB (1024UL * MB)

#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_2M_HUGEPAGE (0x15 << MAP_HUGE_SHIFT) /* 2 ^ 0x15 = 2M */
#define MAP_1G_HUGEPAGE (0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)
#define FLAGS_2M (FLAGS_4K | MAP_2M_HUGEPAGE | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE | MAP_HUGETLB)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_2M_HUGEPAGE | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE | MAP_HUGETLB)
#endif

/* Check validity of various objects */
fpga_result prop_check_and_lock(struct _fpga_properties *prop);
fpga_result handle_check_and_lock(struct _fpga_handle *handle);
fpga_result event_handle_check_and_lock(struct _fpga_event_handle *eh);

#endif // ___FPGA_COMMON_INT_H__
