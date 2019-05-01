// Copyright(c) 2019, Intel Corporation
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

#include <opae/init.h>
#include <opae/utils.h>
#include "pluginmgr.h"
#include "opae_int.h"
#undef __USE_GNU

#include "safe_string/safe_string.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#include <pthread.h>

#define PATH_MAX 1024

__attribute__((constructor)) STATIC void opae_ase_init(void)
{
	char cfg_path[PATH_MAX];
	char *opae_path = getenv("OPAE_PLATFORM_ROOT");
	if (strcpy_s(cfg_path, PATH_MAX, opae_path) != EOK) {
		fprintf(stderr,
				"Could not copy the opae_path successfully.\n");
		return;
	}

	if (strcat_s(cfg_path, PATH_MAX, "/include/opae_ase.cfg") != EOK) {
		fprintf(stderr,
				"Could not copy the cfg_path successfully.\n");
		return;
	}
	setenv("OPAE_EXPLICIT_INITIALIZE", "yes", 0);

	fpgaInitialize(cfg_path);
}

__attribute__((destructor)) STATIC void opae_ase_release(void)
{
	fpga_result res;

	res = fpgaFinalize();
	if (res != FPGA_OK)
		OPAE_ERR("fpgaFinalize: %s", fpgaErrStr(res));
}