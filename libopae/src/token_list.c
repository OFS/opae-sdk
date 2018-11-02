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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "safe_string/safe_string.h"
#include "error_int.h"

#include "token_list_int.h"

/* global list of tokens we've seen */
static struct token_map *token_root;
/* mutex to protect global data structures */
extern pthread_mutex_t global_lock;

/**
 * @brief Add entry to linked list for tokens
 *	Will allocate memory (which is freed by token_cleanup())
 *
 * @param sysfspath
 * @param devpath
 *
 * @return
 */
struct _fpga_token *token_add(const char *sysfspath, const char *devpath)
{
	struct token_map *tmp;
	errno_t e;
	int err = 0;
	uint32_t device_instance;
	uint32_t subdev_instance;
	char *endptr = NULL;
	const char *ptr;

	/* get the device instance id */
	ptr = strchr(sysfspath, '.');
	if (ptr == NULL) {
		FPGA_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	device_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (endptr == ptr) {
		FPGA_MSG("sysfspath does not meet expected format");
		return NULL;
	}
 	/* get the sub-device (FME/Port) instance id */
	ptr = strrchr(sysfspath, '.');
	if (ptr == NULL) {
		FPGA_MSG("sysfspath does not meet expected format");
		return NULL;
	}
 	subdev_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (endptr == ptr) {
		FPGA_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	if (pthread_mutex_lock(&global_lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return NULL;
	}

	/* Prevent duplicate entries. */
	for (tmp = token_root ; NULL != tmp ; tmp = tmp->next) {
		if ((0 == strncmp(sysfspath, tmp->_token.sysfspath,
						SYSFS_PATH_MAX)) &&
				(0 == strncmp(devpath, tmp->_token.devpath,
					      DEV_PATH_MAX))) {
			err = pthread_mutex_unlock(&global_lock);
			if (err) {
				FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
			}
			return &tmp->_token;
		}
	}

	tmp = malloc(sizeof(struct token_map));
	if (!tmp) {
		err = pthread_mutex_unlock(&global_lock);
		if (err) {
			FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
		}
		return NULL;
	}


	/* populate error list */
	tmp->_token.errors = NULL;
	char errpath[SYSFS_PATH_MAX];
	snprintf_s_s(errpath, SYSFS_PATH_MAX, "%s/errors", sysfspath);
	build_error_list(errpath, &tmp->_token.errors);

	/* mark data structure as valid */
	tmp->_token.magic = FPGA_TOKEN_MAGIC;

	/* assign the instances num from above */
	tmp->_token.device_instance = device_instance;
	tmp->_token.subdev_instance = subdev_instance;

	/* deep copy token data */
	e = strncpy_s(tmp->_token.sysfspath, sizeof(tmp->_token.sysfspath),
			sysfspath, SYSFS_PATH_MAX);
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		goto out_free;
	}

	e = strncpy_s(tmp->_token.devpath, sizeof(tmp->_token.devpath),
			devpath, DEV_PATH_MAX);
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		goto out_free;
	}

	tmp->next = token_root;
	token_root = tmp;

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}

	return &tmp->_token;

out_free:
	free(tmp);
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return NULL;
}

/**
 * @ brief Find the token that is the parent of _t
 *
 * @param _t
 *
 * @return parent of _t, or NULL if not found.
 */
struct _fpga_token *token_get_parent(struct _fpga_token *_t)
{
	char *p;
	char spath[SYSFS_PATH_MAX];
	struct token_map *itr;
	int err = 0;
	errno_t e;
	DIR *dir;
	struct dirent *dirent;
	int i;
	int found;

	p = strstr(_t->sysfspath, FPGA_SYSFS_AFU);
	if (!p) // FME objects have no parent.
		return NULL;

	// Find the parent FME device of the specified Port device.
	e = strncpy_s(spath, sizeof(spath),
			_t->sysfspath, sizeof(_t->sysfspath));
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		return NULL;
	}
 	p = strrchr(spath, '/');
	if (!p) {
		FPGA_ERR("Invalid token sysfs path %s", spath);
		return NULL;
	}
	*(p+1) = 0;

	dir = opendir(spath);
	if (!dir) {
		FPGA_ERR("can't open directory: %s", spath);
		return NULL;
	}

	found = 0;
	while ((dirent = readdir(dir)) != NULL) {
		e = strcmp_s(dirent->d_name, sizeof(dirent->d_name),
				".", &i);
		if (e != EOK || i == 0)
			continue;
		e = strcmp_s(dirent->d_name, sizeof(dirent->d_name),
				"..", &i);
		if (e != EOK || i == 0)
			continue;
 		if (strstr(dirent->d_name, FPGA_SYSFS_FME)) {
			e = strcat_s(spath, sizeof(spath),
					dirent->d_name);
			if (e != EOK) {
				FPGA_ERR("strcat_s failed");
				closedir(dir);
				return NULL;
			}
			found = 1;
			break;
		}
	}
 	closedir(dir);
 	if (!found) {
		FPGA_ERR("can't find parent in: %s", spath);
		return NULL;
	}

	if (pthread_mutex_lock(&global_lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return NULL;
	}

	for (itr = token_root ; NULL != itr ; itr = itr->next) {
		if (0 == strncmp(spath, itr->_token.sysfspath,
					SYSFS_PATH_MAX)) {
			err = pthread_mutex_unlock(&global_lock);
			if (err) {
				FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
			}
			return &itr->_token;
		}
	}

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}

	return NULL;
}

/*
 * Clean up remaining entries in linked list
 * Will delete all remaining entries
 */
void token_cleanup(void)
{
	int err = 0;
	struct error_list *p;

	err = pthread_mutex_lock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_lock() failed: %s", strerror(err));
		return;
	}

	if (!token_root)
		goto out_unlock;

	while (token_root->next) {
		struct token_map *tmp = token_root;
		token_root = token_root->next;

		// free error list
		p = tmp->_token.errors;
		while (p) {
			struct error_list *q = p->next;
			free(p);
			p = q;
		}

		// invalidate magic (just in case)
		tmp->_token.magic = FPGA_INVALID_MAGIC;
		free(tmp);
	}

	// free error list
	p = token_root->_token.errors;
	while (p) {
		struct error_list *q = p->next;
		free(p);
		p = q;
	}

	// invalidate magic (just in case)
	token_root->_token.magic = FPGA_INVALID_MAGIC;
	free(token_root);

	token_root = NULL;

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
}
