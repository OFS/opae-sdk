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

#include "safe_string/safe_string.h"

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

	/* mark data structure as valid */
	tmp->_token.magic = FPGA_TOKEN_MAGIC;

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
	int device_instance;
	struct token_map *itr;
	int err = 0;

	p = strstr(_t->sysfspath, FPGA_SYSFS_AFU);
	if (!p) // FME objects have no parent.
		return NULL;

	p = strrchr(_t->sysfspath, '.');
	if (!p)
		return NULL;

	device_instance = atoi(p+1);

	snprintf_s_ii(spath, sizeof(spath),
			SYSFS_FPGA_CLASS_PATH SYSFS_FME_PATH_FMT,
			device_instance, device_instance);

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
	if (pthread_mutex_lock(&global_lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return;
	}

	if (!token_root)
		goto out_unlock;

	while (token_root->next) {
		struct token_map *tmp = token_root;
		token_root = token_root->next;
		// invalidate magic (just in case)
		tmp->_token.magic = FPGA_INVALID_MAGIC;
		free(tmp);
	}

	token_root->_token.magic = FPGA_INVALID_MAGIC;
	free(token_root);
	token_root = NULL;

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
}

