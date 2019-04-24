// Copyright(c) 2017, Intel Corporation
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

#include <errno.h>
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

#include "opae/utils.h"
#include "types_int.h"
#include "wsid_list_int.h"
#include "props.h"

/* Macro for defining symbol visibility */
#define __FPGA_API__ __attribute__((visibility("default")))
#ifdef ASSERT_NOT_NULL_MSG
#undef ASSERT_NOT_NULL_MSG
#define ASSERT_NOT_NULL_MSG(arg, msg)              \
	do {                                       \
		if (!arg) {                        \
			FPGA_MSG(msg);             \
			return FPGA_INVALID_PARAM; \
		}                                  \
	} while (0);

#undef ASSERT_NOT_NULL
#define ASSERT_NOT_NULL(arg) \
	ASSERT_NOT_NULL_MSG(arg, #arg " is NULL")
#endif
static const fpga_guid FPGA_FME_GUID = {
	0xbf, 0xaf, 0x2a, 0xe9, 0x4a, 0x52, 0x46, 0xe3, 0x82, 0xfe,
	0x38, 0xf0, 0xf9, 0xe1, 0x77, 0x64
};

/*
 * Logging functions
 */
enum fpga_loglevel {
	FPGA_LOG_UNDEFINED = -1, /* loglevel not set */
	FPGA_LOG_ERROR = 0,      /* critical errors (always print) */
	FPGA_LOG_MESSAGE,        /* information (i.e. explain return code */
	FPGA_LOG_DEBUG           /* debugging (also needs #define DEBUG 1) */
};

//#define FPGA_DEFAULT_LOGLEVEL 0
/*
 * Convenience macros for printing messages and errors.
 */
#ifdef __SHORT_FILE__
#undef __SHORT_FILE__
#endif // __SHORT_FILE__
#define __SHORT_FILE__               \
({	const char *file = __FILE__; \
	const char *p    = file;     \
	while (*p)                   \
		++p;                 \
	while ((p > file) &&         \
	       ('/' != *p) &&        \
	       ('\\' != *p))         \
		--p;                 \
	if (p > file)                \
		++p;                 \
	p;                           \
})

#ifdef FPGA_MSG
#undef FPGA_MSG
#endif // FPGA_MSG
#define FPGA_MSG(format, ...)\
	fpga_print(FPGA_LOG_MESSAGE, "libfpga %s:%u:%s() : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)

#ifdef FPGA_ERR
#undef FPGA_ERR
#endif // FPGA_ERR
#define FPGA_ERR(format, ...)\
	fpga_print(FPGA_LOG_ERROR, "libfpga %s:%u:%s() **ERROR** : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)

#ifdef FPGA_DBG
#undef FPGA_DBG
#endif // FPGA_DBG
#ifdef LIBFPGA_DEBUG
#define FPGA_DBG(format, ...)\
	fpga_print(FPGA_LOG_DEBUG, "libfpga %s:%u:%s() *DEBUG* : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define FPGA_DBG(format, ...) {}
#endif // LIBFPGA_DEBUG

/* Check validity of various objects */
fpga_result prop_check_and_lock(struct _fpga_properties *prop);
fpga_result handle_check_and_lock(struct _fpga_handle *handle);
fpga_result event_handle_check_and_lock(struct _fpga_event_handle *eh);

#define UNUSED_PARAM(x) ((void)x)

void fpga_print(int loglevel, char *fmt, ...);
struct _fpga_token *token_get_parent(struct _fpga_token *);
fpga_result objectid_for_ase(uint64_t *object_id);

#endif // ___FPGA_COMMON_INT_H__
