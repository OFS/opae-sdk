// Copyright(c) 2022, Intel Corporation
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
#ifndef __OPAE_STD_H__
#define __OPAE_STD_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <glob.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void *opae_malloc(size_t size);
void *opae_calloc(size_t nmemb, size_t size);
void opae_free(void *ptr);

int opae_open(const char *path, int flags);
int opae_open_create(const char *path, int flags, mode_t mode);
int opae_ioctl(int fd, unsigned long request, ...);
int opae_close(int fd);

FILE *opae_fopen(const char *path, const char *mode);
int opae_fclose(FILE *stream);

DIR *opae_opendir(const char *name);
int opae_scandir(const char *dirp,
		 struct dirent ***namelist,
		 int (*filter)(const struct dirent * ),
		 int (*compar)(const struct dirent ** , const struct dirent **));
int opae_closedir(DIR *dirp);

ssize_t opae_readlink(const char *pathname, char *buf, size_t bufsiz);

int opae_lstat(const char *pathname, struct stat *statbuf);

int opae_glob(const char *pattern,
	      int flags,
	      int (*errfunc)(const char *epath, int eerrno),
	      glob_t *pglob);
void opae_globfree(glob_t *pglob);

char *opae_realpath(const char *path, char *resolved_path);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_STD_H__
