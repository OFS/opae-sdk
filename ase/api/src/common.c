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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdarg.h>

#include <opae/utils.h>
#include "common_int.h"

// Buffer Allocation constants
#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_1G_HUGEPAGE	(0x1e << MAP_HUGE_SHIFT)

#define PROTECTION (PROT_READ | PROT_WRITE)
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED)
#define FLAGS_1G (FLAGS | MAP_1G_HUGEPAGE)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#define FLAGS_1G (FLAGS | MAP_1G_HUGEPAGE)
#endif

/* global list of tokens we've seen */
static struct token_map *token_root;
/* global loglevel */
static int g_loglevel = FPGA_LOG_UNDEFINED;

const char __FPGA_API__ *fpgaErrStr(fpga_result e)
{
	switch (e) {
	case FPGA_OK:
		return "success";
	case FPGA_INVALID_PARAM:
		return "invalid parameter";
	case FPGA_BUSY:
		return "resource busy";
	case FPGA_EXCEPTION:
		return "exception";
	case FPGA_NOT_FOUND:
		return "not found";
	case FPGA_NO_MEMORY:
		return "no memory";
	case FPGA_NOT_SUPPORTED:
		return "not supported";
	case FPGA_NO_DRIVER:
		return "no driver available";
	case FPGA_NO_ACCESS:
		return "insufficient privileges";
	default:
		return "unknown error";
	}
}

void fpga_print(int loglevel, char *fmt, ...)
{
	FILE *fp = stdout;
	// FIXME: not thread-safe (may interleave output from different threads)

	if (g_loglevel < 0) { /* loglevel not yet set? */

		/* try to read loglevel from environment */
		char *s = getenv("LIBFPGA_LOG");
		if (s)
			g_loglevel = atoi(s);
#ifndef LIBFGPA_DEBUG
		if (g_loglevel >= FPGA_LOG_DEBUG)
			fprintf(stderr,
				"WARNING: Environment variable LIBFPGA_LOG is "
				"set to output debug\nmessages, "
				"but libfpga was not built with debug "
				"information.\n");
#endif
	}

	if (g_loglevel < 0) /* loglevel still not set? */
		g_loglevel = FPGA_DEFAULT_LOGLEVEL;

	if (loglevel > g_loglevel)
		return;

	if (loglevel == FPGA_LOG_ERROR)
		fp = stderr;

	va_list argp;
	va_start(argp, fmt);
	vfprintf(fp, fmt, argp);
	va_end(argp);

	return;
}

struct _fpga_token *token_get_parent(struct _fpga_token *_t)
{
	if (_t == NULL) {
		printf(" Token is NULL");
	}

	if (0 == memcmp(_t->accelerator_id, FPGA_FME_GUID, sizeof(fpga_guid))) {
		return NULL;
	} else {
		return &aseToken[0];
	}
}
