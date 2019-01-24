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

#include <stdio.h>

#include "safe_string/safe_string.h"
#include "logging.h"
#include "sysfs.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("sysfs: " format, ##__VA_ARGS__)

int file_write_string(const char *path, const char *str, size_t len)
{
	FILE *fp;
	size_t num;

	fp = fopen(path, "w");
	if (!fp)
		return 1;

	num = fwrite(str, 1, len, fp);

	if (!num || ferror(fp)) {
		fclose(fp);
		return 1;
	}

	fclose(fp);

	return 0;
}

int file_read_string(const char *path, char *str, size_t len)
{
	FILE *fp;
	size_t num;

	fp = fopen(path, "r");
	if (!fp)
		return 1;

	num = fread(str, 1, len, fp);

	if (!num || (num == len) || ferror(fp)) {
		fclose(fp);
		return 1;
	}

	fclose(fp);

	if (str[num-1] == '\n')
		--num;

	str[num] = '\0';

	return 0;
}

int file_read_u64(const char *path, uint64_t *value)
{
	char buf[512];
	char *endptr;
	int res;

	res = file_read_string(path, buf, sizeof(buf));
	if (res)
		return res;

	endptr = NULL;
	*value = strtoul(buf, &endptr, 0);

	return (*endptr == '\0') ? 0 : 1;
}

int file_read_fpga_guid(const char *path, fpga_guid guid)
{
	char buf[512];
	int i;
	int res;

	res = file_read_string(path, buf, sizeof(buf));
	if (res)
		return res;

	for (i = 0 ; i < 32 ; i += 2) {
		char tmp = buf[i + 2];
		unsigned octet = 0;

		buf[i + 2] = '\0';
		sscanf_s_u(&buf[i], "%x", &octet);
		guid[i / 2] = (uint8_t)octet;

		buf[i + 2] = tmp;
	}

	return 0;
}

char *cstr_dup(const char *s)
{
	size_t len = strlen(s);
	char *p;

	p = malloc(len+1);
	if (!p)
		return NULL;

	if (strncpy_s(p, len+1, s, len)) {
		LOG("strncpy_s failed.\n");
		free(p);
		return NULL;
	}

	p[len] = '\0';
	return p;
}
