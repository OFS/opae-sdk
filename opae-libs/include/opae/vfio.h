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
 * @file opae/vfio.h
 * @brief APIs to manage a PCIe device via vfio-pci.
 *
 * Presents a simple interface for interacting with a PCIe device that is
 * bound to the vfio-pci driver.
 * See https://kernel.org/doc/Documentation/vfio.txt for a description of
 * vfio-pci.
 *
 * Provides APIs for opening/closing the device, querying info about the
 * MMIO regions of the device, and allocating/mapping and freeing/unmapping
 * DMA buffers.
 */

#ifndef __OPAE_VFIO_H__
#define __OPAE_VFIO_H__

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <linux/vfio.h>

/**
 * IO Virtual Address Range
 *
 * A range of allocatable IOVA offsets. Used for mapping DMA buffers.
 */
struct opae_vfio_iova_range {
	uint64_t start;				/**< Start of this range of offsets. */
	uint64_t end;				/**< End of this range of offsets. */
	uint64_t next_ptr;			/**< The next allocatable offset. */
	struct opae_vfio_iova_range *next;	/**< Pointer to next in list. */
};

/**
 * VFIO group
 *
 * Each device managed by vfio-pci belongs to a VFIO group rooted at
 * /dev/vfio/N, where N is the group number.
 */
struct opae_vfio_group {
	char *group_device;	/**< Full path to the group device. */
	int group_fd;		/**< File descriptor for the group device. */
};

/**
 * MMIO sparse-mappable region info
 *
 * Describes a range of sparse-mappable MMIO, for MMIO ranges that have
 * non-contiguous addresses.
 */
struct opae_vfio_sparse_info {
	uint32_t index;				/**< Region index, from 0. */
	uint32_t offset;			/**< Offset of sparse region. */
	uint32_t size;				/**< Size of sparse region. */
	uint8_t *ptr;				/**< Virtual address of sparse region. */
	struct opae_vfio_sparse_info *next;	/**< Pointer to next in list. */
};

/**
 * MMIO region info
 *
 * Describes a range of mappable MMIO.
 */
struct opae_vfio_device_region {
	uint32_t region_index;				/**< Region index, from 0. */
	uint8_t *region_ptr;				/**< Virtual address of region. */
	size_t region_size;				/**< Size of region. */
	struct opae_vfio_sparse_info *region_sparse;	/**< For sparse regions. */
	struct opae_vfio_device_region *next;		/**< Pointer to next in list. */
};

/**
 * VFIO device
 *
 * Each VFIO device has a file descriptor that is used to query
 * information about the device MMIO regions and config space.
 */
struct opae_vfio_device {
	int device_fd;					/**< Device file descriptor. */
	uint64_t device_config_offset;			/**< Offset of PCIe config space. */
	uint32_t device_num_regions;			/**< Total MMIO region count. */
	struct opae_vfio_device_region *regions;	/**< Region list pointer. */
};

/**
 * System DMA buffer
 *
 * Describes a system memory address space that is capable of DMA.
 */
struct opae_vfio_buffer {
	uint8_t *buffer_ptr;		/**< Buffer virtual address. */
	size_t buffer_size;		/**< Buffer size. */
	uint64_t buffer_iova;		/**< Buffer IOVA address. */
	struct opae_vfio_buffer *next;	/**< Pointer to next in list. */
};

/**
 * OPAE VFIO device abstraction
 *
 * This structure is used to interact with the OPAE VFIO API. It tracks
 * data related to the VFIO container, group, and device. A mutex is
 * provided for thread safety.
 */
struct opae_vfio {
	pthread_mutex_t lock;				/**< For thread safety. */
	char *cont_device;				/**< "/dev/vfio/vfio" */
	char *cont_pciaddr;				/**< PCIe address, eg 0000:00:00.0 */
	int cont_fd;					/**< Container file descriptor. */
	struct opae_vfio_iova_range *cont_ranges;	/**< List of IOVA ranges. */
	struct opae_vfio_group group;			/**< The VFIO device group. */
	struct opae_vfio_device device;			/**< The VFIO device. */
	struct opae_vfio_buffer *cont_buffers;		/**< List of allocated DMA buffers. */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open and populate a VFIO device
 *
 * Opens the PCIe device corresponding to the address given in pciaddr.
 * The device must be bound to the vfio-pci driver prior to opening it.
 * The data structures corresponding to IOVA space, MMIO regions,
 * and DMA buffers are initialized.
 *
 * @param[out] v       Storage for the device info. May be stack-resident.
 * @param[in]  pciaddr The PCIe address of the requested device.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.sh}
 * $ sudo opaevfio -i 0000:00:00.0 -u user -g group
 * @endcode
 *
 * Example
 * @code{.c}
 * opae_vfio v;
 *
 * if (opae_vfio_open(&v, "0000:00:00.0")) {
 *   // handle error
 * }
 * @endcode
 */
int opae_vfio_open(struct opae_vfio *v,
		   const char *pciaddr);

/**
 * Query device MMIO region
 *
 * Retrieves info describing the MMIO region with the given region index.
 * The device structure v must have been previously opened by a call
 * to opae_vfio_open.
 *
 * @param[in] v     The open OPAE VFIO device.
 * @param[in] index The zero-based index of the desired MMIO region.
 * @param[out] ptr  Optional pointer to receive the virtual address
 *                  for the region. Pass NULL to ignore.
 * @param[out] size Optional pointer to receive the size of the MMIO
 *                  region. Pass NULL to ignore.
 * @returns Non-zero on error (including index out-of-range). Zero on success.
 *
 * Example
 * @code{.c}
 * opae_vfio v;
 *
 * uint8_t *fme_virt = NULL;
 * uint8_t *port_virt = NULL;
 * size_t fme_size = 0;
 * size_t port_size = 0;
 *
 * if (opae_vfio_open(&v, "0000:00:00.0")) {
 *   // handle error
 * } else {
 *   opae_vfio_region_get(&v, 0, &fme_virt, &fme_size);
 *   opae_vfio_region_get(&v, 2, &port_virt, &port_size);
 * }
 * @endcode
 */
int opae_vfio_region_get(struct opae_vfio *v,
			 uint32_t index,
			 uint8_t **ptr,
			 size_t *size);

/**
 * Allocate and map system buffer
 *
 * Allocate, map, and retrieve info for a system buffer capable of
 * DMA. Saves an entry in the v->cont_buffers list. If the buffer
 * is not explicitly freed by opae_vfio_buffer_free, it will be
 * freed during opae_vfio_close.
 *
 * mmap is used for the allocation. If the size is greater than 2MB,
 * then the allocation request is fulfilled by a 1GB huge page. Else,
 * if the size is greater than 4096, then the request is fulfilled by
 * a 2MB huge page. Else, the request is fulfilled by the non-huge
 * page pool.
 *
 * @note Allocations from the huge page pool require that huge pages
 * be configured on the system. Huge pages may be configured on the
 * kernel boot command prompt.
 * Example
 * default_hugepagesz=1G hugepagesz=1G hugepages=2 hugepagesz=2M hugepages=8
 *
 * @note Huge pages may also be configured at runtime.
 * Example
 * sudo sh -c 'echo 8 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages'
 * sudo sh -c 'echo 2 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages'
 *
 * @note Be sure that the IOMMU is also enabled using the follow kernel
 * boot command: intel_iommu=on
 *
 * @param[in, out] v    The open OPAE VFIO device.
 * @param[in, out] size A pointer to the requested size. The size
 *                      may be rounded to the next page size prior
 *                      to return from the function.
 * @param[out]     buf  Optional pointer to receive the virtual address
 *                      for the buffer. Pass NULL to ignore.
 * @param[out]     iova Optional pointer to receive the IOVA address
 *                      for the buffer. Pass NULL to ignore.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.c}
 * opae_vfio v;
 *
 * size_t sz;
 * uint8_t *buf_2m_virt = NULL;
 * uint8_t *buf_1g_virt = NULL;
 * uint64_t buf_2m_iova = 0;
 * uint64_t buf_1g_iova = 0;
 *
 * if (opae_vfio_open(&v, "0000:00:00.0")) {
 *   // handle error
 * } else {
 *   sz = 2 * 1024 * 1024;
 *   if (opae_vfio_buffer_allocate(&v,
 *                                 &sz,
 *                                 &buf_2m_virt,
 *                                 &buf_2m_iova)) {
 *     // handle allocation error
 *   }
 *
 *   sz = 1024 * 1024 * 1024;
 *   if (opae_vfio_buffer_allocate(&v,
 *                                 &sz,
 *                                 &buf_1g_virt,
 *                                 &buf_1g_iova)) {
 *     // handle allocation error
 *   }
 * }
 * @endcode
 */
int opae_vfio_buffer_allocate(struct opae_vfio *v,
			      size_t *size,
			      uint8_t **buf,
			      uint64_t *iova);

/**
 * Unmap and free a system buffer
 *
 * The buffer corresponding to buf must have been created by a
 * previous call to opae_vfio_buffer_allocate.
 *
 * @param[in, out] v   The open OPAE VFIO device.
 * @param[in]      buf The virtual address corresponding to
 *                     the buffer to be freed.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.c}
 * size_t sz;
 * uint8_t *buf_2m_virt = NULL;
 * uint64_t buf_2m_iova = 0;
 *
 * sz = 2 * 1024 * 1024;
 * if (opae_vfio_buffer_allocate(&v,
 *                               &sz,
 *                               &buf_2m_virt,
 *                               &buf_2m_iova)) {
 *   // handle allocation error
 * } else {
 *   // use the buffer
 *
 *   if (opae_vfio_buffer_free(&v, buf_2m_virt)) {
 *     // handle free error
 *   }
 * }
 * @endcode
 */
int opae_vfio_buffer_free(struct opae_vfio *v,
			  uint8_t *buf);

/**
 * Release and close a VFIO device
 *
 * The given device pointer must have been previously initialized by
 * opae_vfio_open. Releases all data structures, including any DMA
 * buffer allocations that have not be explicitly freed by
 * opae_vfio_buffer_free.
 *
 * @param[in] v Storage for the device info. May be stack-resident.
 *
 * Example
 * @code{.c}
 * opae_vfio v;
 *
 * if (opae_vfio_open(&v, "0000:00:00.0")) {
 *   // handle error
 * } else {
 *   // interact with the device
 *   ...
 *   // free the device
 *   opae_vfio_close(&v);
 * }
 * @endcode
 *
 * Example
 * @code{.sh}
 * $ sudo opaevfio -r 0000:00:00.0
 * @endcode
 */
void opae_vfio_close(struct opae_vfio *v);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __OPAE_VFIO_H__
