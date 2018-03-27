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

fpga_result fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct stat st;
	uint32_t i = 0;
	fpga_result res = FPGA_OK;

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

fpga_result fpgaClearError(fpga_token token, uint32_t error_num)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct stat st;
	uint32_t i = 0;
	uint64_t value = 0;
	fpga_result res = FPGA_OK;

	// TODO: make thread-safe

	struct error_list *p = _token->errors;
	while (p) {
		if (i == error_num) {
			if (!p->info.can_clear) {
				FPGA_MSG("can't clear error '%s'", p->info.name);
				return FPGA_NOT_SUPPORTED;
			}
			// read current error value
			res = fpgaReadError(token, error_num, &value);
			if (res != FPGA_OK)
				return res;

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

fpga_result fpgaClearAllErrors(fpga_token token)
{
	UNUSED_PARAM(token);
	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	uint32_t i = 0;

	if (!error_info) {
		FPGA_MSG("error_info is NULL");
		return FPGA_INVALID_PARAM;
	}

	// TODO: make thread-safe

	// TODO: should we populate this if user hasn't created a
	//       properties structure for this token?

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
const char *errors_exclude[] = {
	"revision",
	"uevent",
	"power",
	"clear"
};
#define NUM_ERRORS_EXCLUDE (sizeof(errors_exclude) / sizeof(char*))

/* returns the number of error entries added to `list` */
uint32_t build_error_list(const char *path, struct error_list **list)
{
	struct dirent *de;
	DIR *dir;
	struct stat st;
	char basedir[FILENAME_MAX];
	int len = strlen(path);
	uint32_t n = 0;
	unsigned int i;
	struct error_list **el = list;

	if (len+1 > FILENAME_MAX) {
		FPGA_MSG("path too long");
		return 0;
	}

	strncpy(basedir, path, FILENAME_MAX-1);
	basedir[len++] = '/';

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
			if (strcmp(de->d_name, errors_exclude[i]) == 0) {  // FIXME: SAFE?
				break;
			}
		}
		if (i < NUM_ERRORS_EXCLUDE)
			continue;

		// build absolute path
		strncpy(basedir + len, de->d_name, FILENAME_MAX - len);

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
		strncpy_s(new_entry->info.name, FPGA_ERROR_NAME_MAX, de->d_name, FILENAME_MAX);
		strncpy_s(new_entry->error_file, SYSFS_PATH_MAX, basedir, FILENAME_MAX);
		new_entry->next = NULL;
		// see if error can be cleared (currently only errors called "errors" corresponding
		// to a "clear" file can)
		new_entry->info.can_clear = false;
		if (strcmp(de->d_name, "errors") == 0) {    // FIXME: SAFE
			strncpy_s(basedir + len, FILENAME_MAX - len, "clear", sizeof("clear"));
			// try accessing clear file
			if (lstat(basedir, &st) != -1) {
				new_entry->info.can_clear = true;
				strncpy_s(new_entry->clear_file, SYSFS_PATH_MAX, basedir, FILENAME_MAX);
			}
		}

		if (!new_entry->info.can_clear) {
			memset_s(new_entry->clear_file, sizeof(new_entry->clear_file), 0);
		}

		// find end of list
		while (*el)
			el = &(*el)->next;

		// append
		*el = new_entry;
		el = &new_entry->next;
	}	

	return n;
}


uint32_t count_error_files(const char *path) {
	return build_error_list(path, NULL);
}