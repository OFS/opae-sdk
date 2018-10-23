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

/*
 * sysfs.c : sysfs utilities
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "common_int.h"
#include "opae/fpga.h"
#include "log.h"
#include "safe_string/safe_string.h"


fpga_result sysfs_write_1(fpga_token token, const char *path)
{
	int fd;
	char buf[SYSFS_PATH_MAX];
	char buf2[SYSFS_PATH_MAX] = {'\0'};
	struct _fpga_token *tok = (struct _fpga_token *)token;

	if (*path == '/') {
		snprintf_s_s(buf, SYSFS_PATH_MAX, "%s", path);
	} else {
		char *star = strchr(path, '*');
		if (!star) {
			snprintf_s_ss(buf, SYSFS_PATH_MAX, "%s/%s",
				      tok->sysfspath, path);
		} else {
			strcpy_s(buf2, SYSFS_PATH_MAX, path);
			star = strchr(buf2, '*');
			char *numb = strrchr(tok->sysfspath, '.');
			if (!numb || !isxdigit(numb[1])) {
				dlog("sysfs path corrupt!\n");
				return FPGA_NOT_FOUND;
			}
			*star = numb[1];
			snprintf_s_ss(buf, SYSFS_PATH_MAX, "%s/%s",
				      tok->sysfspath, buf2);
		}
	}

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		dlog("open(%s) failed\n", buf);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		dlog("seek failed\n");
		goto out_close;
	}

	ssize_t count = write(fd, "1\n", 2);

	close(fd);
	if (count != 2) {

		dlog("Write only wrote %d bytes\n", (int)count);
	}

	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}
