// Copyright(c) 2017-2020, Intel Corporation
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
#include <sys/stat.h>

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
	int err = 0;
	uint32_t device_instance;
	uint32_t subdev_instance;
	char *endptr = NULL;
	const char *ptr;
	size_t len;

	/* get the device instance id */
	ptr = strchr(sysfspath, '.');
	if (ptr == NULL) {
		OPAE_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	device_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (endptr == ptr) {
		OPAE_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	/* get the sub-device (FME/Port) instance id */
	ptr = strrchr(sysfspath, '.');
	if (ptr == NULL) {
		OPAE_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	subdev_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (endptr == ptr) {
		OPAE_MSG("sysfspath does not meet expected format");
		return NULL;
	}

	if (pthread_mutex_lock(&global_lock)) {
		OPAE_MSG("Failed to lock global mutex");
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
				OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
			}
			return &tmp->_token;
		}
	}

	tmp = malloc(sizeof(struct token_map));
	if (!tmp) {
		err = pthread_mutex_unlock(&global_lock);
		if (err) {
			OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
		}
		return NULL;
	}


	/* populate error list */
	tmp->_token.errors = NULL;
	char errpath[SYSFS_PATH_MAX] = { 0, };

	len = strnlen(sysfspath, sizeof(errpath) - 1);
	strncpy(errpath, sysfspath, len + 1);
	len = strnlen("/errors", sizeof(errpath) - len);
	strncat(errpath, "/errors", len + 1);

	build_error_list(errpath, &tmp->_token.errors);

	/* mark data structure as valid */
	tmp->_token.magic = FPGA_TOKEN_MAGIC;

	/* assign the instances num from above */
	tmp->_token.device_instance = device_instance;
	tmp->_token.subdev_instance = subdev_instance;

	/* deep copy token data */
	len = strnlen(sysfspath, SYSFS_PATH_MAX - 1);
	strncpy(tmp->_token.sysfspath, sysfspath, len + 1);
	len = strnlen(devpath, DEV_PATH_MAX - 1);
	strncpy(tmp->_token.devpath, devpath, len + 1);

	tmp->next = token_root;
	token_root = tmp;

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}

	return &tmp->_token;
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
	char spath[SYSFS_PATH_MAX] = { 0, };
	char rpath[PATH_MAX] = { 0, };
	struct token_map *itr;
	int err = 0;
	char *rptr = NULL;
	fpga_result res = FPGA_OK;

	p = strstr(_t->sysfspath, FPGA_SYSFS_AFU);
	if (!p) // FME objects have no parent.
		return NULL;

	res = sysfs_get_fme_path(_t->sysfspath, spath);
	if (res) {
		OPAE_ERR("Could not find fme path for token: %s",
			 _t->sysfspath);
		return NULL;
	}

	if (pthread_mutex_lock(&global_lock)) {
		OPAE_MSG("Failed to lock global mutex");
		return NULL;
	}

	for (itr = token_root ; NULL != itr ; itr = itr->next) {
		rptr = realpath(itr->_token.sysfspath, rpath);
		if (rptr && !strncmp(spath, rptr, SYSFS_PATH_MAX)) {
			err = pthread_mutex_unlock(&global_lock);
			if (err) {
				OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
			}
			return &itr->_token;
		}
	}

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
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
		OPAE_ERR("pthread_mutex_lock() failed: %s", strerror(err));
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
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
}
