// Copyright(c) 2020, Intel Corporation
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
 * @file opaevfio.h
 * @brief Functions 
 *
 */

#ifndef __OPAEVFIO_H__
#define __OPAEVFIO_H__

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <linux/vfio.h>

struct opae_vfio_iova_range {
	uint64_t start;
	uint64_t end;
	uint64_t next_ptr;
	struct opae_vfio_iova_range *next;
};

struct opae_vfio_group {
	char *group_device;
	int group_fd;
};

struct opae_vfio_sparse_info {
	uint32_t index;
	uint32_t offset;
	uint32_t size;
	uint8_t *ptr;
	struct opae_vfio_sparse_info *next;
};

struct opae_vfio_device_region {
	uint32_t region_index;
	uint8_t *region_ptr;
	size_t region_size;
	struct opae_vfio_sparse_info *region_sparse;
	struct opae_vfio_device_region *next;
};

struct opae_vfio_device {
	int device_fd;
	uint64_t device_config_offset;
	uint32_t device_num_regions;
	struct opae_vfio_device_region *regions;
};

struct opae_vfio_buffer {
	uint8_t *buffer_ptr;
	size_t buffer_size;
	uint64_t buffer_iova;
	struct opae_vfio_buffer *next;
};

struct opae_vfio_container {
	pthread_mutex_t lock;
	char *cont_device;
	char *cont_pciaddr;
	int cont_fd;
	struct opae_vfio_iova_range *cont_ranges;
	struct opae_vfio_group group;
	struct opae_vfio_device device;
	struct opae_vfio_buffer *cont_buffers;
};

#ifdef __cplusplus
extern "C" {
#endif

int opae_vfio_open(struct opae_vfio_container *c,
		   const char *device,
		   const char *pciaddr);

int opae_vfio_region_get(struct opae_vfio_container *c,
			 uint32_t index,
			 uint8_t **ptr,
			 size_t *size);

int opae_vfio_buffer_allocate(struct opae_vfio_container *c,
			      size_t *size,
			      uint8_t **buf,
			      uint64_t *iova);

int opae_vfio_buffer_free(struct opae_vfio_container *c,
			  uint8_t *buf);

void opae_vfio_close(struct opae_vfio_container *c);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __OPAEVFIO_H__
