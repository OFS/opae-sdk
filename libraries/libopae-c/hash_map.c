// Copyright(c) 2022, Intel Corporation
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
#include "mock/opae_std.h"
#include "hash_map.h"

fpga_result opae_hash_map_init(opae_hash_map *hm,
			       uint32_t num_buckets,
			       uint32_t hash_seed,
			       uint32_t (*key_hash)(uint32_t num_buckets,
						    uint32_t hash_seed,
						    void *key),
			       int (*key_compare)(void *keya, void *keyb),
			       void (*key_cleanup)(void *key),
			       void (*value_cleanup)(void *value))
{
	if (!hm || !key_hash || !key_compare) {
		OPAE_ERR("NULL pointer(s)");
		return FPGA_INVALID_PARAM;
	}

	memset(hm, 0, sizeof(*hm));

	hm->buckets = (opae_hash_map_item **)
			opae_calloc(num_buckets,
				    sizeof(opae_hash_map_item *));
	if (!hm->buckets) {
		OPAE_ERR("calloc() failed");
		return FPGA_NO_MEMORY;
	}

	hm->num_buckets = num_buckets;
	hm->hash_seed = hash_seed;
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

	if (!hm) {
		OPAE_ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	item = opae_hash_map_alloc_item(key, value);
	if (!item) {
		OPAE_ERR("malloc() failed");
		return FPGA_NO_MEMORY;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	list = hm->buckets[key_hash];
	if (!list) {
		// Bucket was empty. Fill it.
		hm->buckets[key_hash] = item;
		return FPGA_OK;
	}

	// Bucket not empty. Do we have a key collision?
	while (list) {
		if (!hm->key_compare(key, list->key)) {
			// Key collision.
			opae_free(item);
			if (hm->value_cleanup)
				hm->value_cleanup(list->value);
			list->value = value; // Replace value only.
			return FPGA_OK;
		}
		list = list->next;
	}

	// No key collision. Add item to the list.
	item->next = hm->buckets[key_hash];
	hm->buckets[key_hash] = item;

	return FPGA_OK;
}

fpga_result opae_hash_map_find(opae_hash_map *hm,
			       void *key,
			       void **value)
{
	uint32_t key_hash;
	opae_hash_map_item *list;

	if (!hm) {
		OPAE_ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	list = hm->buckets[key_hash];

	while (list) {
		if (!hm->key_compare(key, list->key)) {
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
		OPAE_ERR("NULL pointer");
		return FPGA_INVALID_PARAM;
	}

	key_hash = hm->key_hash(hm->num_buckets,
				hm->hash_seed,
				key);

	list = hm->buckets[key_hash];
	if (!list)
		return FPGA_NOT_FOUND;

	while (list) {
		if (!hm->key_compare(key, list->key))
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
		hm->key_cleanup(list->key);
	if (hm->value_cleanup)
		hm->value_cleanup(list->value);
	opae_free(list);

	return FPGA_OK;
}

fpga_result opae_hash_map_destroy(opae_hash_map *hm)
{
	uint32_t i;

	if (!hm) {
		OPAE_ERR("NULL pointer");
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
				hm->key_cleanup(trash->key);
			if (hm->value_cleanup)
				hm->value_cleanup(trash->value);
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

// https://en.wikipedia.org/wiki/MurmurHash
static inline uint32_t murmur_32_scramble(uint32_t k)
{
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

// https://en.wikipedia.org/wiki/MurmurHash
uint32_t murmur3_32_string_hash(uint32_t num_buckets,
				uint32_t hash_seed,
				void *key)
{
	uint8_t *key8;
	size_t i;
	size_t len;
	uint32_t h;
	uint32_t k;

	key8 = (uint8_t *)key;
	len = strlen((const char *)key);
	h = hash_seed;

	for (i = len >> 2 ; i ; --i) {
		memcpy(&k, key8, sizeof(uint32_t));
		key8 += sizeof(uint32_t);
		h ^= murmur_32_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}

	k = 0;
	for (i = len & 3 ; i ; --i) {
		k <<= 8;
		k |= key8[i - 1];
	}
	h ^= murmur_32_scramble(k);

	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h % num_buckets;
}
