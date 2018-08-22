// Copyright(c) 2018, Intel Corporation
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

#ifndef __OPAE_OPAE_INT_H__
#define __OPAE_OPAE_INT_H__

#include <string.h>
#include <errno.h>

#include "log_int.h"

/* Macro for defining symbol visibility */
//#define __FPGA_API__ __attribute__((visibility("default")))
//#define __FIXME_MAKE_VISIBLE__ __attribute__((visibility("default")))

/*
 * Check if argument is NULL and return FPGA_INVALID_PARAM and a message
 */
#define ASSERT_NOT_NULL_MSG(arg, msg)              \
	do {                                       \
		if (!arg) {                        \
			FPGA_MSG(msg);             \
			return FPGA_INVALID_PARAM; \
		}                                  \
	} while (0)

#define ASSERT_NOT_NULL(arg) \
	ASSERT_NOT_NULL_MSG(arg, #arg " is NULL")


#define UNUSED_PARAM(x) ((void)x)


#define opae_mutex_lock(__res, __mtx_ptr)                         \
	({                                                        \
		(__res) = pthread_mutex_lock(__mtx_ptr);          \
		if (__res)                                        \
			OPAE_ERR("pthread_mutex_lock failed: %s", \
					strerror(errno));         \
		__res;                                            \
	})

#define opae_mutex_unlock(__res, __mtx_ptr)                         \
	({                                                          \
		(__res) = pthread_mutex_unlock(__mtx_ptr);          \
		if (__res)                                          \
			OPAE_ERR("pthread_mutex_unlock failed: %s", \
					strerror(errno));           \
		__res;                                              \
	})

#endif // ___OPAE_OPAE_INT_H__
