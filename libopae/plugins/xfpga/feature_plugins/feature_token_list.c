// Copyright(c) 2018-2019, Intel Corporation
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
#include <uuid/uuid.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#undef _GNU_SOURCE

#include "safe_string/safe_string.h"
#include "types_int.h"
#include "feature_token_list_int.h"

/* global list of tokens we've seen */
static struct _fpga_feature_token *ftoken_root;

extern pthread_mutex_t global_lock;
/**
 * @brief Add entry to linked list for feature tokens
 *	Will allocate memory (which is freed by feature_token_cleanup())
 *
 * @param type
 * @param guid
 * @param handle
 *
 * @return
 */
struct _fpga_feature_token *feature_token_add(uint32_t type, uint32_t mmio_num, fpga_guid guid,
					      uint64_t offset, fpga_handle handle)
{
	struct _fpga_feature_token *tmp;
	errno_t e;
	int err = 0;

	if (pthread_mutex_lock(&global_lock)) {
		FPGA_ERR("Failed to lock feature token mutex");
		return NULL;
	}

	/* Prevent duplicate entries. */
	for (tmp = ftoken_root; NULL != tmp; tmp = tmp->next) {
		if ((uuid_compare(guid, tmp->feature_guid)) == 0) {
			err = pthread_mutex_unlock(&global_lock);
			if (err) {
				FPGA_ERR("pthread_mutex_unlock() failed: %s",
					 strerror(err));
			}
			printf("feature_token_add found token in the list, return immedaitely\n");
			return tmp;
		}
	}

	tmp = (struct _fpga_feature_token *)malloc(
		sizeof(struct _fpga_feature_token));
	if (NULL == tmp) {
		FPGA_ERR("Failed to allocate memory for fhandle");
		goto out_unlock;
	}

	uuid_clear(tmp->feature_guid);
	tmp->magic = FPGA_FEATURE_TOKEN_MAGIC;
	tmp->feature_type = type;
	tmp->mmio_num = mmio_num;
	tmp->csr_offset = offset;
	tmp->handle = handle;
	tmp->next = NULL;

	e = memcpy_s(tmp->feature_guid, sizeof(fpga_guid), guid,
		     sizeof(fpga_guid));

	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		goto out_free;
	}

	tmp->next = ftoken_root;
	ftoken_root = tmp;

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
		free(tmp);
		tmp = NULL;
	}

	return tmp;

out_free:
	free(tmp);

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return NULL;
}

/*
 * Clean up remaining entries in linked list
 * Will delete all remaining entries
 */
void feature_token_cleanup(void)
{
	int err = 0;
	struct _fpga_feature_token *current = ftoken_root;
	err = pthread_mutex_lock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_lock() failed: %s", strerror(err));
		return;
	}

	if (!ftoken_root)
		goto out_unlock;

	while (current) {
		struct _fpga_feature_token *tmp = current;
		current = current->next;

		// invalidate magic (just in case)
		tmp->magic = FPGA_INVALID_MAGIC;
		free(tmp);
		tmp = NULL;
	}

	ftoken_root = NULL;

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
}
