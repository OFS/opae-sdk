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

/**
 * @file opae/hash_map.h
 * @brief A general-purpose hybrid array/list hash map implementation.
 *
 * Presents a generic interface for mapping key objects to value objects.
 * Both keys and values may be arbitrary data structures. The user supplies
 * the means by which the hash of values is generated and by which the
 * keys are compared to each other.
 */

#ifndef __OPAE_HASH_MAP_H__
#define __OPAE_HASH_MAP_H__
#include <stdint.h>
#include <stdbool.h>
#include <opae/types_enum.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/** Flags used to initialize a hash map.
 *
 * OPAE_HASH_MAP_UNIQUE_KEYSPACE says that the user provides
 * a guarantee that the key space is truly unique. In other words, when
 * the provided hash function for keys A and B returns the same bucket
 * index, the key comparison function when comparing A and B will never
 * return a result saying that the keys are equal in value. This is helpful
 * in situations where the key space is guaranteed to produce unique values,
 * for example a memory allocator. When the key space is guaranteed to be
 * unique, opae_hash_map_add() can implement a small performance improvement.
 */
typedef enum _opae_hash_map_flags {
	OPAE_HASH_MAP_UNIQUE_KEYSPACE = (1u << 0)
} opae_hash_map_flags;

/**
 * List link item.
 *
 * This structure provides the association between key and value.
 * When the supplied hash function for keys A and B returns the same
 * bucket index, both A and B can co-exist on the same list rooted
 * at the bucket index.
 */
typedef struct _opae_hash_map_item {
	void *key;
	void *value;
	struct _opae_hash_map_item *next;
} opae_hash_map_item;

/**
 * Hash map object.
 *
 * This structure defines the internals of the hash map. Each of the
 * parameters supplied to opae_hash_map_init() is stored in the structure.
 * All parameters are required, except key_cleanup and value_cleanup,
 * which may optionally be NULL.
 */
typedef struct _opae_hash_map {
	uint32_t num_buckets;
	uint32_t hash_seed;
	opae_hash_map_item **buckets;
	int flags;
	void *cleanup_context; ///< Optional second parameter to key_cleanup and value_cleanup
	uint32_t (*key_hash)(uint32_t num_buckets,	   ///< (required)
			     uint32_t hash_seed,
			     void *key);
	int (*key_compare)(void *keya, void *keyb);	   ///< (required)
	void (*key_cleanup)(void *key, void *context);	   ///< (optional)
	void (*value_cleanup)(void *value, void *context); ///< (optional)
} opae_hash_map;

/**
 * Initialize a hash map
 *
 * Populates the hash map data structure and allocates the buckets
 * array.
 *
 * @param[out] hm            A pointer to the storage for the hash map object.
 * @param[in]  num_buckets   The desired size of the buckets array. Each array
 *                           entry may be empty (NULL), or may contain a list
 *                           of opae_hash_map_item structures for which the given
 *                           key_hash function returned the same key hash value.
 * @param[in]  hash_seed     A seed value used during key hash computation. This
 *                           value will be the hash_seed parameter to the key hash
 *                           function.
 * @param[in]  flags         Initialization flags. See opae_hash_map_flags.
 * @param[in]  key_hash      A pointer to a function that produces the hash value,
 *                           given the number of buckets, the hash seed, and the key.
 *                           Valid values are between 0 and num_buckets - 1, inclusively.
 * @param[in]  key_compare   A pointer to a function that compares two keys. The return
 *                           value is similar to that of strcmp(), where a negative value
 *                           means that keya < keyb, 0 means that keya == keyb, and a positive
 *                           values means that keya > keyb.
 * @param[in]  key_cleanup   A pointer to a function that is called when a key is being
 *                           removed from the map. This function is optional and may be
 *                           NULL. When supplied, the function is responsible for freeing
 *                           any resources allocated when the key was created.
 * @param[in]  value_cleanup A pointer to a function that is called when a value is
 *                           being removed from the map. This function is optional and may
 *                           be NULL. When supplied, the function is responsible for freeing
 *                           any resources allocated when the value was created.
 * @returns FPGA_OK on success, FPGA_INVALID_PARAM if any of the required parameters are
 *          NULL, or FPGA_NO_MEMORY if the bucket array could not be allocated.
 */
fpga_result opae_hash_map_init(opae_hash_map *hm,
			       uint32_t num_buckets,
			       uint32_t hash_seed,
			       int flags,
			       uint32_t (*key_hash)(uint32_t num_buckets,
						    uint32_t hash_seed,
						    void *key),
			       int (*key_compare)(void *keya, void *keyb),
			       void (*key_cleanup)(void *key, void *context),
			       void (*value_cleanup)(void *value, void *context));

/**
 * Map a key to a value
 *
 * Inserts a mapping from key to value in the given hash map object.
 * Subsequent calls to opae_hash_map_find() that are given the key
 * will retrieve the value.
 *
 * @param[in, out] hm    A pointer to the storage for the hash map object.
 * @param[in]      key   The hash map key.
 * @param[in]      value The hash map value.
 * @returns FPGA_OK on success, FPGA_INVALID_PARAM if hm is NULL, FPGA_NO_MEMORY
 *          if malloc() fails when allocating the list item, or FPGA_INVALID_PARAM
 *          if the key hash produced by key_hash is out of bounds.
 */
fpga_result opae_hash_map_add(opae_hash_map *hm,
			      void *key,
			      void *value);

/**
 * Retrieve the value for a given key
 *
 * Given a key that was previously passed to opae_hash_map_add(), retrieve
 * its associated value.
 *
 * @param[in] hm    A pointer to the storage for the hash map object.
 * @param[in] key   The hash map key.
 * @param[in] value A pointer to receive the hash map value.
 * @returns FPGA_OK on success, FPGA_INVALID_PARAM if hm is NULL or if the
 *          key hash produced by key_hash is out of bounds, or FPGA_NOT_FOUND
 *          if the given key was not found in the hash map.
 */
fpga_result opae_hash_map_find(opae_hash_map *hm,
			       void *key,
			       void **value);

/**
 * Remove a key/value association
 *
 * Given a key that was previously passed to opae_hash_map_add(), remove the
 * key and its associated value, calling the cleanup functions as needed.
 *
 * @param[in, out] hm    A pointer to the storage for the hash map object.
 * @param[in]      key   The hash map key.
 * @returns FPGA_OK on success, FPGA_INVALID_PARAM when hm is NULL or when the
 *          key hash produced by key_hash is out of bounds, or FPGA_NOT_FOUND if
 *          the key is not found in the hash map.
 */ 
fpga_result opae_hash_map_remove(opae_hash_map *hm,
				 void *key);

/**
 * Tear down a hash map
 *
 * Given a hash map that was previously initialized by opae_hash_map_init(),
 * destroy the hash map, releasing all keys, values, and the bucket array.
 *
 * @param[in, out] hm A pointer to the storage for the hash map object.
 * @returns FPGA_OK on success or FPGA_INVALID_PARAM is hm is NULL.
 */
fpga_result opae_hash_map_destroy(opae_hash_map *hm);

/**
 * Determine whether a hash map is empty
 *
 * @param[in] hm A pointer to the storage for the hash map object.
 * @returns true if there are no key/value mappings present, false
 *          otherwise.
 */
bool opae_hash_map_is_empty(opae_hash_map *hm);

/**
 * Convenience hash function for arbitrary pointers/64-bit values.
 *
 * Simply converts the key to a uint64_t and then performs the
 * modulus operation with the configured num_buckets. hash_seed is
 * unused.
 */
uint32_t opae_u64_key_hash(uint32_t num_buckets,
			   uint32_t hash_seed,
			   void *key);

/**
 * Convenience key comparison function for 64-bit values.
 *
 * Simply converts the key pointers to uint64_t's and performs
 * unsigned integer comparison.
 */
int opae_u64_key_compare(void *keya, void *keyb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_HASH_MAP_H__
