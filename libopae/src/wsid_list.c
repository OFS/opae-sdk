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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "wsid_list_int.h"

/* mutex to protect global data structures */
extern pthread_mutex_t global_lock;

/**
 * @brief Add entry to linked list for WSIDs
 *        Will allocate memory (which is freed by wsid_del() or wsid_cleanup())
 * @param root
 * @param wsid
 * @param addr
 * @param phys
 * @param len
 * @param offset
 *
 * @return true if success, false otherwise
 */
bool wsid_add(struct wsid_map **root,
	      uint64_t wsid,
	      uint64_t addr,
	      uint64_t phys,
	      uint64_t len,
	      uint64_t offset,
	      uint64_t index,
	      int flags)
{
	struct wsid_map *tmp = malloc(sizeof(struct wsid_map));

	if (!tmp)
		return false;

	tmp->wsid   = wsid;
	tmp->addr   = addr;
	tmp->phys   = phys;
	tmp->len    = len;
	tmp->offset = offset;
	tmp->index  = index;
	tmp->flags  = flags;
	tmp->next   = *root;

	*root = tmp;
	return true;
}

/**
 * @brief Remove entry from linked list
 *
 * @param root
 * @param wsid
 *
 * @return true if success, false otherwise
 */
bool wsid_del(struct wsid_map **root, uint64_t wsid)
{
	struct wsid_map *tmp = *root;

	if (!*root)
		return false; /* empty list */

	if ((*root)->wsid == wsid) { /* first entry */
		*root = (*root)->next;
		free(tmp);
		return true;
	}

	while (tmp->next && tmp->next->wsid != wsid) { /* find */
		tmp = tmp->next;
	}

	if (!tmp->next)
		return false; /* not found */

	struct wsid_map *tmp2 = tmp->next;
	tmp->next = tmp->next->next;
	free(tmp2);

	return true;
}

/**
 * @brief Clean up remaining entries in linked list
 *        Will delete all remaining entries
 *
 * @param root
 */
void wsid_cleanup(struct wsid_map **root, void (*clean)(struct wsid_map *))
{
	if (!*root)
		return;

	while ((*root)->next) {
		struct wsid_map *tmp = *root;
		*root = (*root)->next;
		if (clean)
			clean(tmp);
		free(tmp);
	}

	if (clean)
		clean(*root);
	free(*root);
	*root = NULL;
}

/**
 * @ brief Find entry in linked list
 *
 * @param root
 * @param wsid
 *
 * @return
 */
struct wsid_map *wsid_find(struct wsid_map *root, uint64_t wsid)
{
	struct wsid_map *tmp = root;

	while (tmp && tmp->wsid != wsid)
		tmp = tmp->next;

	return tmp;
}

/**
 * @ brief Find entry in linked list
 *
 * @param root
 * @param index
 *
 * @return
 */
struct wsid_map *wsid_find_by_index(struct wsid_map *root, uint32_t index)
{
	struct wsid_map *tmp = root;

	while (tmp && tmp->index != index)
		tmp = tmp->next;

	return tmp;
}

