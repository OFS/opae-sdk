// Copyright(c) 2019-2020, Intel Corporation
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

#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <opae/log.h>

#include "bits_utils.h"

fpga_result opae_bitstream_get_json_string(json_object *parent,
					   const char *name,
					   char **value)
{
	json_object *obj = NULL;
	const char *s;
	size_t len;
	char *p;

	if (!json_object_object_get_ex(parent,
				       name,
				       &obj)) {
		return FPGA_EXCEPTION;
	}

	if (!json_object_is_type(obj, json_type_string)) {
		OPAE_ERR("metadata: \"%s\" key not string", name);
		return FPGA_EXCEPTION;
	}

	s = json_object_get_string(obj);

	len = strlen(s);

	*value = malloc(len + 1);
	if (!*value) {
		OPAE_ERR("malloc failed");
		return FPGA_NO_MEMORY;
	}

	strncpy(*value, s, len + 1);
	p = *value;
	p[len] = '\0';

	return FPGA_OK;
}

fpga_result opae_bitstream_get_json_int(json_object *parent,
					const char *name,
					int *value)
{
	json_object *obj = NULL;

	if (!json_object_object_get_ex(parent,
				       name,
				       &obj)) {
		return FPGA_EXCEPTION;
	}

	if (!json_object_is_type(obj, json_type_int)) {
		return FPGA_EXCEPTION;
	}

	*value = json_object_get_int(obj);
	return FPGA_OK;
}

fpga_result opae_bitstream_get_json_double(json_object *parent,
					   const char *name,
					   double *value)
{
	json_object *obj = NULL;

	if (!json_object_object_get_ex(parent,
				       name,
				       &obj)) {
		return FPGA_EXCEPTION;
	}

	if (!json_object_is_type(obj, json_type_double)) {
		return FPGA_EXCEPTION;
	}

	*value = json_object_get_double(obj);
	return FPGA_OK;
}

STATIC bool opae_bitstream_path_invalid_chars(const char *path,
					      size_t len)
{
	while (*path) {
		int ch = *path;

		// check for non-printable chars
		if (!isprint(ch))
			return true;

		// check for URL encoding
		if ((ch == '%') &&
		    (len >= 3) &&
		    (isxdigit(*(path+1)) && isxdigit(*(path+2))))
			return true;

		++path;
		--len;
	}

	return false;
}

STATIC bool opae_bitstream_path_not_file(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) < 0)
		return true; // can't determine

	if (!S_ISREG(sb.st_mode))
		return true;

	return false;
}

STATIC bool opae_bitstream_path_contains_dotdot(const char *path,
						size_t len)
{
	if (len >= 3) {
		// check for ../ at the start of the path.
		if ((*path == '.') &&
		    (*(path + 1) == '.') &&
		    (*(path + 2) == '/'))
			return true;
	} else if (len == 2) {
		// check for ".."
		if ((*path == '.') &&
		    (*(path + 1) == '.'))
			return true;
	}

	while (*path) {

		if (len >= 4) {
			// check for /../
			if ((*path == '/') &&
			    (*(path + 1) == '.') &&
			    (*(path + 2) == '.') &&
			    (*(path + 3) == '/'))
				return true;
		} else if (len == 3) {
			// check for /.. at the end
			if ((*path == '/') &&
			    (*(path + 1) == '.') &&
			    (*(path + 2) == '.'))
				return true;
		}

		++path;
		--len;
	}

	return false;
}

STATIC bool opae_bitstream_path_contains_symlink(const char *path,
						 size_t len)
{
	char component[PATH_MAX] = { 0, };
	struct stat stat_buf;
	char *pslash;

	strncpy(component, path, len);
	component[len] = '\0';

	if (component[0] == '/') {
		// absolute path

		pslash = realpath(path, component);

		// If the result of conversion through realpath() is different
		// than the original path, then the original must have
		// contained a symlink.
		if (strcmp(component, path)) {
			return true;
		}

	} else {
		// relative path

		pslash = strrchr(component, '/');

		while (pslash) {

			if (fstatat(AT_FDCWD, component,
				    &stat_buf, AT_SYMLINK_NOFOLLOW)) {
				OPAE_ERR("fstatat failed.");
				return true;
			}

			if (S_ISLNK(stat_buf.st_mode))
				return true;

			*pslash = '\0';
			pslash = strrchr(component, '/');
		}

		if (fstatat(AT_FDCWD, component,
			    &stat_buf, AT_SYMLINK_NOFOLLOW)) {
			OPAE_ERR("fstatat failed.");
			return true;
		}

		if (S_ISLNK(stat_buf.st_mode))
			return true;

	}

	return false;
}

bool opae_bitstream_path_is_valid(const char *path,
				  uint32_t flags)
{
	size_t len;

	// check for NULL / empty string
	if (!path || (*path == '\0'))
		return false;

	len = strlen(path);

	if (opae_bitstream_path_invalid_chars(path, len))
		return false;

	if (opae_bitstream_path_not_file(path))
		return false;

	if ((flags & OPAE_BITSTREAM_PATH_NO_PARENT) &&
	    opae_bitstream_path_contains_dotdot(path, len))
		return false;

	if ((flags & OPAE_BITSTREAM_PATH_NO_SYMLINK) &&
	    opae_bitstream_path_contains_symlink(path, len))
		return false;

	return true;
}
