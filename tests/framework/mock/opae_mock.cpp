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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "opae_std.h"
#include "mock/test_system.h"

// TODO: popen(), pclose()
// TODO: need opae_fclose()?
// TODO: need opae_free()?

void *opae_malloc(size_t size)
{
  return opae::testing::test_system::instance()->malloc(size);
}

void *opae_calloc(size_t nmemb, size_t size)
{
  return opae::testing::test_system::instance()->calloc(nmemb, size);
}

void opae_free(void *ptr)
{
// TODO
	free(ptr);
}

int opae_open(const char *path, int flags)
{
  return opae::testing::test_system::instance()->open(path, flags);
}

int opae_open_create(const char *path, int flags, mode_t mode)
{
  return opae::testing::test_system::instance()->open(path, flags, mode);
}

int opae_ioctl(int fd, unsigned long request, ...)
{
	va_list argp;
	int res;

	va_start(argp, request);
	res = opae::testing::test_system::instance()->ioctl(fd, request, argp);
	va_end(argp);

	return res;
}

int opae_close(int fd)
{
  return opae::testing::test_system::instance()->close(fd);
}

FILE *opae_fopen(const char *path, const char *mode)
{
  return opae::testing::test_system::instance()->fopen(path, mode);
}

int opae_fclose(FILE *stream)
{
// TODO
	return fclose(stream);
}

DIR *opae_opendir(const char *name)
{
  return opae::testing::test_system::instance()->opendir(name);
}

int opae_scandir(const char *dirp,
		 struct dirent ***namelist,
		 int (*filter)(const struct dirent * ),
		 int (*compar)(const struct dirent ** , const struct dirent **))
{
  return opae::testing::test_system::instance()->scandir(dirp, namelist,
                                                         filter, compar);
}

int opae_closedir(DIR *dirp)
{
// TODO
	return closedir(dirp);
}

ssize_t opae_readlink(const char *pathname, char *buf, size_t bufsize)
{
  return opae::testing::test_system::instance()->readlink(pathname, buf, bufsize);
}

int opae_lstat(const char *pathname, struct stat *statbuf)
{
// TODO
	return lstat(pathname, statbuf);
}

int opae_glob(const char *pattern,
	      int flags,
	      int (*errfunc)(const char *epath, int eerrno),
	      glob_t *pglob)
{
  return opae::testing::test_system::instance()->glob(pattern, flags,
                                                      errfunc, pglob);
}

void opae_globfree(glob_t *pglob)
{
	if (pglob->gl_pathv)
		globfree(pglob);
}

char *opae_realpath(const char *path, char *resolved_path)
{
  return opae::testing::test_system::instance()->realpath(path, resolved_path);
}
