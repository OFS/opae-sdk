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

#ifndef __OPAE_LOG_H__
#define __OPAE_LOG_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <opae/types.h>

/*
* Convenience macros for printing messages and errors.
*/
#ifdef __SHORT_FILE__
#undef __SHORT_FILE__
#endif // __SHORT_FILE__
#define __SHORT_FILE__                                         \
	({                                                     \
	const char *file = __FILE__;                           \
	const char *p = file;                                  \
while (*p)                                                     \
	++p;                                                   \
while ((p > file) && ('/' != *p) && ('\\' != *p))              \
	--p;                                                   \
if (p > file)                                                  \
	++p;                                                   \
	p;                                                     \
})

#ifdef OPAE_MSG
#undef OPAE_MSG
#endif // OPAE_MSG
#define OPAE_MSG(format, ...)                                     \
	opae_print(OPAE_LOG_MESSAGE, "%s:%u:%s() : " format "\n", \
	__SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef OPAE_ERR
#undef OPAE_ERR
#endif // OPAE_ERR
#define OPAE_ERR(format, ...)                                     \
	opae_print(OPAE_LOG_ERROR,                                \
	"%s:%u:%s() **ERROR** : " format "\n",                    \
	__SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef OPAE_DBG
#undef OPAE_DBG
#endif // OPAE_DBG
#ifdef LIBOPAE_DEBUG
#define OPAE_DBG(format, ...)                                    \
	opae_print(OPAE_LOG_DEBUG,                               \
	"%s:%u:%s() *DEBUG* : " format "\n",                     \
	__SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define OPAE_DBG(format, ...)                                    \
{	}
#endif // LIBOPAE_DEBUG

#ifndef FPGA_MSG
#define FPGA_MSG OPAE_MSG
#endif // FPGA_MSG

#ifndef FPGA_ERR
#define FPGA_ERR OPAE_ERR
#endif // FPGA_ERR

#ifndef FPGA_DBG
#define FPGA_DBG OPAE_DBG
#endif // FPGA_DBG

/*
* Logging functions
*/
enum opae_loglevel {
	OPAE_LOG_ERROR = 0, /* critical errors (always print) */
	OPAE_LOG_MESSAGE,   /* information (i.e. explain return code */
	OPAE_LOG_DEBUG      /* debugging (also needs #define DEBUG 1) */
};

#define OPAE_DEFAULT_LOGLEVEL OPAE_LOG_ERROR

#ifndef FPGA_DEFAULT_LOGLEVEL
#define FPGA_DEFAULT_LOGLEVEL OPAE_DEFAULT_LOGLEVEL
#endif // FPGA_DEFAULT_LOGLEVEL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void opae_print(int loglevel, const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_LOG_H__
