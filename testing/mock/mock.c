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
 * Mock up driver interactions for testing
 *
 * Involves redefining ioctl(), open(), close(), others?
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <opae/types.h>
#include "common_int.h"
#include "intel-fpga.h"
#include <assert.h>
#include <stdint.h>
#include <safe_string/safe_string.h>
#include "c_test_system.h"

#define __USE_GNU
#include <dlfcn.h>

#define MAX_FD 1024
#define MAX_STRLEN 256
#define FPGA_MOCK_IOVA 0xDECAFBADDEADBEEF
#define FPGA_MOCK_NUM_UMSGS 8
#define FPGA_MOCK_DEV_PATH "/tmp"
#define MOCK_SYSFS_FPGA_CLASS_PATH "/tmp/class/fpga"
#define FPGA_FME_DEV_PREFIX "intel-fpga-fme."
#define FPGA_PORT_DEV_PREFIX "intel-fpga-port."
#define HASH_SUFFIX ".gbshash"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

#undef FPGA_ERR
#define FPGA_ERR(fmt, ...) \
	printf("MOCK ERROR " fmt "\n", ## __VA_ARGS__)

#undef FPGA_DBG
#ifdef LIBFPGA_DEBUG
#define FPGA_DBG(fmt, ...) \
	printf("MOCK DEBUG " fmt "\n", ## __VA_ARGS__)
#else
#define FPGA_DBG(fmt, ...) {}
#endif




int ioctl(int fd, unsigned long request, ...)
{
	va_list argp;
	va_start(argp, request);
	int res = opae_test_ioctl(fd, request, argp);
	va_end(argp);
	return res;
}

int open(const char* path, int flags, ...)
{
	int fd = -1;
	if (flags & O_CREAT) {
		va_list argp;
		va_start(argp, flags);
		mode_t arg = va_arg(argp, mode_t);
		fd = opae_test_open_create(path, flags, arg);
		va_end(argp);
	}
	else {
		fd = opae_test_open(path, flags);
	}
	return fd;
}

int close(int fd)
{
	return opae_test_close(fd);
}

DIR *opendir(const char *name)
{
	return opae_test_opendir(name);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
	return opae_test_readlink(pathname, buf, bufsiz);
}

int __xstat(int ver, const char *pathname, struct stat *buf)
{
	return opae_test_xstat(ver, pathname, buf);
}

int __lxstat(int ver, const char *pathname, struct stat *buf)
{
	return opae_test_xstat(ver, pathname, buf);
}

