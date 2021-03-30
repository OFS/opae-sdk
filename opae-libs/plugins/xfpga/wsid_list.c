// Copyright(c) 2017-2018, Intel Corporation
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

/*
 * The code here assumes the caller handles any required mutexes.
 * The logic here is not thread safe on its own.
 */

/**
 * @brief Initialize a wsid tracker hash table
 * @param n_hash_buckets
 *
 * @return
 */
struct wsid_tracker *wsid_tracker_init(uint32_t n_hash_buckets)
{
	if (!n_hash_buckets || (n_hash_buckets > 16384))
		return NULL;

	struct wsid_tracker *root = malloc(sizeof(struct wsid_tracker));
	if (!root)
		return NULL;

	root->n_hash_buckets = n_hash_buckets;
	root->table = calloc(n_hash_buckets, sizeof(struct wsid_map *));
	if (!root->table) {
		free(root);
		return NULL;
	}

	return root;
}

/**
 * @brief Map WSID to hash bucket index
 * @param root
 * @param wsid
 *
 * @return bucket index
 */
static inline uint32_t wsid_hash(struct wsid_tracker *root, uint64_t wsid)
{
	uint64_t h = wsid % 17659;
	return h % root->n_hash_buckets;
}


/**
 * @brief Add entry to WSID tracker
 *        Will allocate memory (which is freed by wsid_del() or
 *        wsid_tracker_cleanup())
 * @param root
 * @param wsid
 * @param addr
 * @param phys
 * @param len
 * @param offset
 *
 * @return true if success, false otherwise
 */
bool wsid_add(struct wsid_tracker *root,
	      uint64_t wsid,
	      uint64_t addr,
	      uint64_t phys,
	      uint64_t len,
	      uint64_t offset,
	      uint64_t index,
	      int flags)
{
	uint32_t idx = wsid_hash(root, wsid);
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
	tmp->next   = root->table[idx];

	root->table[idx] = tmp;
	return true;
}

/**
 * @brief Remove entry from tracker
 *
 * @param root
 * @param wsid
 *
 * @return true if success, false otherwise
 */
bool wsid_del(struct wsid_tracker *root, uint64_t wsid)
{
	uint32_t idx = wsid_hash(root, wsid);
	struct wsid_map *tmp = root->table[idx];

	if (!tmp)
		return false; /* empty list */

	if (tmp->wsid == wsid) { /* first entry */
		root->table[idx] = root->table[idx]->next;
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
void wsid_tracker_cleanup(struct wsid_tracker *root,
			  void (*clean)(struct wsid_map *))
{
	uint32_t idx;

	if (!root)
		return;

	for (idx = 0; idx < root->n_hash_buckets; idx += 1) {
		struct wsid_map *tmp = root->table[idx];

		while (tmp) {
			struct wsid_map *tmp2 = tmp->next;
			if (clean)
				clean(tmp);
			free(tmp);
			tmp = tmp2;
		}
	}

	free(root->table);
	free(root);
}

/**
 * @ brief Find entry in linked list
 *
 * @param root
 * @param wsid
 *
 * @return
 */
struct wsid_map *wsid_find(struct wsid_tracker *root, uint64_t wsid)
{
	uint32_t idx = wsid_hash(root, wsid);
	struct wsid_map *tmp = root->table[idx];

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
struct wsid_map *wsid_find_by_index(struct wsid_tracker *root, uint32_t index)
{
    /*
     * The hash table isn't set up for finding by index, but this search is
     * used only for MMIO spaces, which should have a small number of entries.
     */
	uint32_t idx;
	for (idx = 0; idx < root->n_hash_buckets; idx += 1) {
		struct wsid_map *tmp = root->table[idx];

		while (tmp && tmp->index != index)
			tmp = tmp->next;

		if (tmp)
			return tmp;
	}

	return NULL;
}

