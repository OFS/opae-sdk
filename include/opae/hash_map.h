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
#ifndef __OPAE_HASH_MAP_H__
#define __OPAE_HASH_MAP_H__
#include <stdint.h>
#include <stdbool.h>
#include <opae/types_enum.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum _opae_hash_map_flags {
	OPAE_HASH_MAP_UNIQUE_KEYSPACE = (1u << 0)
} opae_hash_map_flags;

typedef struct _opae_hash_map_item {
	void *key;
	void *value;
	struct _opae_hash_map_item *next;
} opae_hash_map_item;

typedef struct _opae_hash_map {
	uint32_t num_buckets;
	uint32_t hash_seed;
	opae_hash_map_item **buckets;
	int flags;
	void *cleanup_context;
	uint32_t (*key_hash)(uint32_t num_buckets,	   ///< required
			     uint32_t hash_seed,
			     void *key);
	int (*key_compare)(void *keya, void *keyb);	   ///< required
	void (*key_cleanup)(void *key, void *context);	   ///< optional
	void (*value_cleanup)(void *value, void *context); ///< optional
} opae_hash_map;

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

fpga_result opae_hash_map_add(opae_hash_map *hm,
			      void *key,
			      void *value);

fpga_result opae_hash_map_find(opae_hash_map *hm,
			       void *key,
			       void **value);

fpga_result opae_hash_map_remove(opae_hash_map *hm,
				 void *key);

fpga_result opae_hash_map_destroy(opae_hash_map *hm);

bool opae_hash_map_is_empty(opae_hash_map *hm);

uint32_t opae_u64_key_hash(uint32_t num_buckets,
			   uint32_t hash_seed,
			   void *key);

int opae_u64_key_compare(void *keya, void *keyb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_HASH_MAP_H__
