// Copyright(c) 2019-2022, Intel Corporation
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

#include <stdio.h>

#include "logging.h"
#include "sysfs.h"
#include "mock/opae_std.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("sysfs: " format, ##__VA_ARGS__)

int file_write_string(const char *path, const char *str, size_t len)
{
	FILE *fp;
	size_t num;

	fp = opae_fopen(path, "w");
	if (!fp)
		return 1;

	num = fwrite(str, 1, len, fp);

	if (!num || ferror(fp)) {
		opae_fclose(fp);
		return 1;
	}

	opae_fclose(fp);

	return 0;
}

int file_read_string(const char *path, char *str, size_t len)
{
	FILE *fp;
	size_t num;

	fp = opae_fopen(path, "r");
	if (!fp)
		return 1;

	num = fread(str, 1, len - 1, fp);

	if (!num || ferror(fp)) {
		opae_fclose(fp);
		return 1;
	}

	opae_fclose(fp);

	str[num] = '\0';
	if (str[num - 1] == '\n')
		str[num - 1] = '\0';

	return 0;
}
