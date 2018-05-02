// Copyright(c) 2017-2018, Intel Corporation
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

#ifndef __FPGA_SRC_LOG_H__
#define __FPGA_SRC_LOG_H__

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
	fpga_print(FPGA_LOG_MESSAGE, "libopae-c %s:%u:%s() : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)

#ifdef FPGA_ERR
#undef FPGA_ERR
#endif // FPGA_ERR
#define FPGA_ERR(format, ...)\
	fpga_print(FPGA_LOG_ERROR, "libopae-c %s:%u:%s() **ERROR** : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)

#ifdef FPGA_DBG
#undef FPGA_DBG
#endif // FPGA_DBG
#ifdef LIBFPGA_DEBUG
#define FPGA_DBG(format, ...)\
	fpga_print(FPGA_LOG_DEBUG, "libopae-c %s:%u:%s() *DEBUG* : " format "\n",\
		   __SHORT_FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define FPGA_DBG(format, ...) {}
#endif // LIBFPGA_DEBUG

/*
 * Logging functions
 */
enum fpga_loglevel {
	FPGA_LOG_UNDEFINED = -1, /* loglevel not set */
	FPGA_LOG_ERROR = 0,      /* critical errors (always print) */
	FPGA_LOG_MESSAGE,        /* information (i.e. explain return code */
	FPGA_LOG_DEBUG           /* debugging (also needs #define DEBUG 1) */
};
#define FPGA_DEFAULT_LOGLEVEL 0

void fpga_print(int loglevel, char *fmt, ...);

#endif // ___FPGA_SRC_LOG_H__
