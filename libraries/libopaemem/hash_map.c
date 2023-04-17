// Copyright(c) 2022-2023, Intel Corporation
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

#include <opae/log.h>
#include <opae/hash_map.h>
#include "mock/opae_std.h"

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(x) ((void)(x))
#endif // UNUSED_PARAM

#define __SHORT_FILE__                                    \
({                                                        \
	const char *file = __FILE__;                      \
	const char *p = file;                             \
	while (*p)                                        \
		++p;                                      \
	while ((p > file) && ('/' != *p) && ('\\' != *p)) \
		--p;                                      \
	if (p > file)                                     \
		++p;                                      \
	p;                                                \
})

#define ERR(format, ...)                               \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format, \
	__SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

fpga_result opae_hash_map_init(opae_hash_map *hm,
			       uint32_t num_buckets,
			       uint32_t hash_seed,
			       int flags,
			       uint32_t (*key_hash)(uint32_t num_buckets,
						    uint32_t hash_seed,
						    void *key),
			       int (*key_compare)(void *keya, void *keyb),
			       void (*key_cleanup)(void *key, void *context),
			       void (*value_cleanup)(void *value, void *context))
{
	if (!hm || !key_hash || !key_compare) {
		ERR("NULL pointer(s)");
		return FPGA_INVALID_PARAM;
	}

	memset(hm, 0, sizeof(*hm));

	hm->buckets = (opae_hash_map_item **)
			opae_calloc(num_buckets,
				    sizeof(opae_hash_map_item *));
	if (!hm->buckets) {
		ERR("calloc() failed");
		return FPGA_NO_MEMORY;
	}

	hm->num_buckets = num_buckets;
	hm->hash_seed = hash_seed;
	hm->flags = flags;
	hm->key_hash = key_hash;
	hm->key_compare = key_compare;
	hm->key_cleanup = key_cleanup;
	hm->value_cleanup = value_cleanup;

	return FPGA_OK;
}

STATIC opae_hash_map_item *
opae_hash_map_alloc_item(void *key, void *value)
{
	opae_hash_map_item *item;
	item = (opae_hash_map_item *)
		opae_malloc(sizeof(opae_hash_map_item));
	if (item) {
		item->key = key;
		item->value = value;
		item->next = NULL;
	}
	return item;
}

fpga_result opae_hash_map_add(opae_hash_map *hm,
			      void *key,
			      void *value)
{
	uint32_t key_hash;
	opae_hash_map_item *item;
	opae_hash_map_item *list;
	opae_hash_map_item *prev;

	if (!hm) {
		ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	if (key_hash >= hm->num_buckets) {
		ERR("key hash returned %u which is "
		    "greater or equal num_buckets(%u)\n",
		    key_hash, hm->num_buckets);
		return FPGA_INVALID_PARAM;
	}

	item = opae_hash_map_alloc_item(key, value);
	if (!item) {
		ERR("malloc() failed");
		return FPGA_NO_MEMORY;
	}

	list = hm->buckets[key_hash];
	if (!list) {
		// Bucket was empty. Fill it.
		hm->buckets[key_hash] = item;
		return FPGA_OK;
	}

	if (hm->flags & OPAE_HASH_MAP_UNIQUE_KEYSPACE) {
		// The user has guaranteed us a unique keyspace.
		// In this case, we are sure that no key collisions
		// will occur on add. As an optimization, just add new
		// entries to the head of the list.
		item->next = hm->buckets[key_hash];
		hm->buckets[key_hash] = item;
	} else {
		// Bucket not empty. Do we have a key collision?
		prev = NULL;
		while (list) {
			int res;

			if (hm->key_compare == opae_u64_key_compare)
				res = opae_u64_key_compare(key, list->key);
			else
				res = hm->key_compare(key, list->key);

			if (!res) {
				// Key collision.
				opae_free(item);
				if (hm->value_cleanup)
					hm->value_cleanup(list->value,
							  hm->cleanup_context);
				list->value = value; // Replace value only.
				return FPGA_OK;
			}
			prev = list;
			list = list->next;
		}

		// No key collision. Add item to the end of list.
		prev->next = item;
	}

	return FPGA_OK;
}

fpga_result opae_hash_map_find(opae_hash_map *hm,
			       void *key,
			       void **value)
{
	uint32_t key_hash;
	opae_hash_map_item *list;

	if (!hm) {
		ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	if (key_hash >= hm->num_buckets) {
		ERR("key hash returned %u which is "
		    "greater or equal num_buckets(%u)\n",
		    key_hash, hm->num_buckets);
		return FPGA_INVALID_PARAM;
	}

	list = hm->buckets[key_hash];

	while (list) {
		int res;

		if (hm->key_compare == opae_u64_key_compare)
			res = opae_u64_key_compare(key, list->key);
		else
			res = hm->key_compare(key, list->key);

		if (!res) {
			if (value)
				*value = list->value;
			return FPGA_OK;
		}
		list = list->next;
	}

	return FPGA_NOT_FOUND;
}

fpga_result opae_hash_map_remove(opae_hash_map *hm,
				 void *key)
{
	uint32_t key_hash;
	opae_hash_map_item *prev = NULL;
	opae_hash_map_item *list;

	if (!hm) {
		ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	if (key_hash >= hm->num_buckets) {
		ERR("key hash returned %u which is "
		    "greater or equal num_buckets(%u)\n",
		    key_hash, hm->num_buckets);
		return FPGA_INVALID_PARAM;
	}

	list = hm->buckets[key_hash];
	if (!list)
		return FPGA_NOT_FOUND;

	while (list) {
		int res;

		if (hm->key_compare == opae_u64_key_compare)
			res = opae_u64_key_compare(key, list->key);
		else
			res = hm->key_compare(key, list->key);

		if (!res)
			break;
		prev = list;
		list = list->next;
	}

	if (!list)
		return FPGA_NOT_FOUND;

	if (!prev) {
		// Key found at head of list.
		hm->buckets[key_hash] = list->next;
	} else {
		prev->next = list->next;
	}

	if (hm->key_cleanup)
		hm->key_cleanup(list->key, hm->cleanup_context);
	if (hm->value_cleanup)
		hm->value_cleanup(list->value, hm->cleanup_context);
	opae_free(list);

	return FPGA_OK;
}

fpga_result opae_hash_map_destroy(opae_hash_map *hm)
{
	uint32_t i;

	if (!hm) {
		ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0 ; i < hm->num_buckets ; ++i) {
		opae_hash_map_item *item;
		item = hm->buckets[i];
		while (item) {
			opae_hash_map_item *trash;
			trash = item;
			item = item->next;
			if (hm->key_cleanup)
				hm->key_cleanup(trash->key, hm->cleanup_context);
			if (hm->value_cleanup)
				hm->value_cleanup(trash->value, hm->cleanup_context);
			opae_free(trash);
		}
	}

	opae_free(hm->buckets);
	memset(hm, 0, sizeof(*hm));

	return FPGA_OK;
}

bool opae_hash_map_is_empty(opae_hash_map *hm)
{
	uint32_t i;

	for (i = 0 ; i < hm->num_buckets ; ++i) {
		if (hm->buckets[i])
			return false;
	}

	return true;
}

uint32_t opae_u64_key_hash(uint32_t num_buckets,
			   uint32_t hash_seed,
			   void *key)
{
	UNUSED_PARAM(hash_seed);
	uint64_t ukey = (uint64_t)key;
	return (uint32_t)(ukey % num_buckets);
}

inline int opae_u64_key_compare(void *keya, void *keyb)
{
	uint64_t a = (uint64_t)keya;
	uint64_t b = (uint64_t)keyb;

	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
}
