// Copyright(c) 2018-2020, Intel Corporation
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

#include "safe_string/safe_string.h"

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#include "common_int.h"
#include "opae/error.h"

#include "error_int.h"

#define INJECT_ERROR "inject_error"

fpga_result __XFPGA_API__ xfpga_fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct stat st;
	uint32_t i = 0;
	fpga_result res = FPGA_OK;

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	struct error_list *p = _token->errors;
	while (p) {
		if (i == error_num) {
			// test if file exists
			if (stat(p->error_file, &st) == -1) {
				FPGA_MSG("can't stat %s", p->error_file);
				return FPGA_EXCEPTION;
			}
			res = sysfs_read_u64(p->error_file, value);
			if (res != FPGA_OK) {
				FPGA_MSG("can't read error file '%s'", p->error_file);
				return res;
			}

			return FPGA_OK;
		}
		i++;
		p = p->next;
	}

	FPGA_MSG("error %d not found", error_num);
	return FPGA_NOT_FOUND;
}

fpga_result __XFPGA_API__
xfpga_fpgaClearError(fpga_token token, uint32_t error_num)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct stat st;
	uint32_t i = 0;
	uint64_t value = 0;
	fpga_result res = FPGA_OK;

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	struct error_list *p = _token->errors;
	while (p) {
		if (i == error_num) {
			if (!p->info.can_clear) {
				FPGA_MSG("can't clear error '%s'", p->info.name);
				return FPGA_NOT_SUPPORTED;
			}

			if (strcmp(p->info.name, INJECT_ERROR) == 0) {
				value = 0;
			} else {
				// read current error value
				res = xfpga_fpgaReadError(token, error_num, &value);
				if (res != FPGA_OK)
					return res;
			}

			// write to 'clear' file
			if (stat(p->clear_file, &st) == -1) {
				FPGA_MSG("can't stat %s", p->clear_file);
				return FPGA_EXCEPTION;
			}
			res = sysfs_write_u64(p->clear_file, value);
			if (res != FPGA_OK) {
				FPGA_MSG("can't write clear file '%s'", p->clear_file);
				return res;
			}
			return FPGA_OK;
		}
		i++;
		p = p->next;
	}

	FPGA_MSG("error info %d not found", error_num);
	return FPGA_NOT_FOUND;
}

fpga_result __XFPGA_API__ xfpga_fpgaClearAllErrors(fpga_token token)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	uint32_t i = 0;
	fpga_result res = FPGA_OK;

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	struct error_list *p = _token->errors;
	while (p) {
		// if error can be cleared
		if (p->info.can_clear) {
			// clear error
			res = xfpga_fpgaClearError(token, i);
			if (res != FPGA_OK)
				return res;
		}
		i++;
		p = p->next;
	}

	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	uint32_t i = 0;

	if (!error_info) {
		FPGA_MSG("error_info is NULL");
		return FPGA_INVALID_PARAM;
	}

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	struct error_list *p = _token->errors;
	while (p) {
		if (i == error_num) {
			memcpy_s(error_info, sizeof(struct fpga_error_info), &p->info, sizeof(struct fpga_error_info));
			return FPGA_OK;
		}
		i++;
		p = p->next;
	}

	FPGA_MSG("error info %d not found", error_num);
	return FPGA_NOT_FOUND;
}

/* files and directories to ignore when looking for errors */
#define NUM_ERRORS_EXCLUDE 4
const char *errors_exclude[NUM_ERRORS_EXCLUDE] = {
	"revision",
	"uevent",
	"power",
	"clear"
};

/* files that can be cleared by writing their value to them */
#define NUM_ERRORS_CLEARABLE 6
const char *errors_clearable[] = {
	"pcie0_errors",
	"pcie1_errors",
	"warning_errors",
	"inject_error",
	"fme_errors",
	"errors"
};

/* Walks the given directory and adds error entries to `list`.
 * This function is called during enumeration when adding tokens to
 * the global tokens list. When tokens are cloned, their error
 * lists are only shallowly copied (which works because errors of
 * a token never change).
 * Note that build_error_list() does not check for dupliates; if
 * called again on the same list, it will add all found errors again.
 * Returns the number of error entries added to `list` */
uint32_t
build_error_list(const char *path, struct error_list **list)
{
	struct dirent *de;
	DIR *dir;
	struct stat st;
	char basedir[FILENAME_MAX];
	int len = strlen(path);
	int subpath_len = 0;
	uint32_t n = 0;
	unsigned int i;
	struct error_list **el = list;
	errno_t err;

	// add 3 to the len
	// 1 for the '/' char
	// 1 for the minimum length of a file appended
	// 1 for null string to terminate
	// if we go over now, then leave without doing anything else
	if (len+3 > FILENAME_MAX) {
		FPGA_MSG("path too long");
		return 0;
	}

	err = strncpy_s(basedir, FILENAME_MAX-3, path, len);
	if (err != EOK) {
		FPGA_MSG("strncpy_s() failed with return value %u", err);
		return 0;
	}
	basedir[len++] = '/';
	// now we've added one to length

	dir = opendir(path);
	if (!dir) {
		FPGA_MSG("unable to open %s", path);
		return 0;
	}

	while ((de = readdir(dir))) {
		// skip hidden ('.*') files (includes "." and "..")
		if (de->d_name[0] == '.')
			continue;

		// skip names on blacklist
		for (i = 0; i < NUM_ERRORS_EXCLUDE; i++) {
			if (strcmp(de->d_name, errors_exclude[i]) == 0) {
				break;
			}
		}
		if (i < NUM_ERRORS_EXCLUDE)
			continue;

		subpath_len = strlen(de->d_name);

		// check if the result abs path is longer than  our max
		if (len + subpath_len > FILENAME_MAX) {
			FPGA_MSG("Error path length is too long");
			continue;
		}

		// build absolute path
		// dmax (arg2) is restricted max length of resulting dest,
		// including null - it must also be at least smax+1 (arg4)
		err = strncpy_s(basedir + len, FILENAME_MAX - len - 1,
				de->d_name, subpath_len);
		if (err != EOK) {
			FPGA_MSG("strncpy_s() failed with return value %u", err);
			continue;
		}

		// try accessing file/dir
		if (lstat(basedir, &st) == -1) {
			FPGA_MSG("can't stat %s", basedir);
			continue;
		}

		// skip symlinks
		if (S_ISLNK(st.st_mode))
			continue;

		// recursively dive into subdirectories
		if (S_ISDIR(st.st_mode)) {
			n += build_error_list(basedir, el);
			continue;
		}

		// not blacklisted, not hidden, accessible, no symlink, no dir -> count and append it!
		n++;
		if (!el)	// no list
			continue;

		// append error info to list
		struct error_list *new_entry = malloc(sizeof(struct error_list));
		if (!new_entry) {
			FPGA_MSG("can't allocate memory");
			n--;
			break;
		}
		err = strncpy_s(new_entry->info.name, FPGA_ERROR_NAME_MAX, de->d_name, FILENAME_MAX);
		if (err != EOK) {
			FPGA_MSG("strncpy_s() failed with return value %u", err);
			n--;
			free(new_entry);
			new_entry = NULL;
			break;
		}
		err = strncpy_s(new_entry->error_file, SYSFS_PATH_MAX, basedir, FILENAME_MAX);
		if (err != EOK) {
			FPGA_MSG("strncpy_s() failed with return value %u", err);
			n--;
			free(new_entry);
			new_entry = NULL;
			break;
		}
		new_entry->next = NULL;
		// Errors can be cleared:
		//   * if the name is "errors" and there is a file called "clear" (generic case), OR
		//   * if the name is in the "errors_clearable" table
		new_entry->info.can_clear = false;
		if (strcmp(de->d_name, "errors") == 0 &&
			!stat(FPGA_SYSFS_CLASS_PATH_INTEL, &st)) {
			err = strncpy_s(basedir + len, FILENAME_MAX - len, "clear", sizeof("clear"));
			if (err != EOK) {
				FPGA_MSG("strncpy_s() failed with return value %u", err);
				n--;
				free(new_entry);
				new_entry = NULL;
				break;
			}
			// try accessing clear file
			if (lstat(basedir, &st) != -1) {
				new_entry->info.can_clear = true;
				err = strncpy_s(new_entry->clear_file, SYSFS_PATH_MAX, basedir, FILENAME_MAX);
				if (err != EOK) {
					FPGA_MSG("strncpy_s() failed with return value %u", err);
					n--;
					free(new_entry);
					new_entry = NULL;
					break;
				}
			}
		} else {
			for (i = 0; i < NUM_ERRORS_CLEARABLE; i++) {
				if (strcmp(de->d_name, errors_clearable[i]) == 0) {
					err = strncpy_s(basedir + len, FILENAME_MAX - len, de->d_name, FILENAME_MAX);
					if (err != EOK) {
						FPGA_MSG("strncpy_s() failed with return value %u", err);
						n--;
						free(new_entry);
						new_entry = NULL;
						break;
					}
					// try accessing clear file
					if (lstat(basedir, &st) != -1) {
						new_entry->info.can_clear = true;
						err = strncpy_s(new_entry->clear_file, SYSFS_PATH_MAX, basedir, FILENAME_MAX);
						if (err != EOK) {
							FPGA_MSG("strncpy_s() failed with return value %u", err);
							n--;
							free(new_entry);
							new_entry = NULL;
							break;
						}
					}
				}
			}
		}

		if (new_entry && !new_entry->info.can_clear) {
			err = memset_s(new_entry->clear_file, sizeof(new_entry->clear_file), 0);
			// the first two arguments passed in to memset_s are
			// always valid - no need to check for error code
		}

		// find end of list
		while (*el)
			el = &(*el)->next;

		// append
		if (new_entry)
			*el = new_entry;
		el = &new_entry->next;
	}
	closedir(dir);

	return n;
}


uint32_t count_error_files(const char *path)
{
	return build_error_list(path, NULL);
}
