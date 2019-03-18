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

/*
 * Mock up driver interactions that call into test_system API for testing
 *
 * Involves redefining ioctl(), open(), close(), others?
 */

#include <stdio.h>
#include <fcntl.h>
#include <safe_string/safe_string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include "c_test_system.h"

int ioctl(int fd, unsigned long request, ...) {
  va_list argp;
  va_start(argp, request);
  int res = opae_test_ioctl(fd, request, argp);
  va_end(argp);
  return res;
}

int open(const char *path, int flags, ...) {
  int fd = -1;
  if (flags & O_CREAT) {
    va_list argp;
    va_start(argp, flags);
    mode_t arg = va_arg(argp, mode_t);
    fd = opae_test_open_create(path, flags, arg);
    va_end(argp);
  } else {
    fd = opae_test_open(path, flags);
  }
  return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
  return opae_test_read(fd, buf, count);
}

FILE * fopen(const char *path, const char *mode) {
  return opae_test_fopen(path, mode);
}

FILE * popen(const char *cmd, const char *type) {
  return opae_test_popen(cmd, type);
}

int pclose(FILE *stream) {
  return opae_test_pclose(stream);
}

int close(int fd) { return opae_test_close(fd); }

DIR *opendir(const char *name) { return opae_test_opendir(name); }

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  return opae_test_readlink(pathname, buf, bufsiz);
}

int __xstat(int ver, const char *pathname, struct stat *buf) {
  return opae_test_xstat(ver, pathname, buf);
}

int __lxstat(int ver, const char *pathname, struct stat *buf) {
  return opae_test_xstat(ver, pathname, buf);
}

int scandir(const char *__restrict __dir,
            struct dirent ***__restrict __namelist,
            int (*__selector)(const struct dirent *),
            int (*__cmp)(const struct dirent **, const struct dirent **)) {
  return opae_test_scandir(__dir, __namelist, __selector, __cmp);
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
  return opae_test_sched_setaffinity(pid, cpusetsize, mask);
}

int glob(const char *pattern, int flags,
         int (*errfunc)(const char *epath, int eerrno), glob_t *pglob) {
  return opae_test_glob(pattern, flags, errfunc, pglob);
}

char *realpath(const char *inp, char *dst) {
	return opae_test_realpath(inp, dst);
}
