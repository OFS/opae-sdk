/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Header File for host exerciser cache DFL User API
 *
 * Copyright (C) 2023 Intel Corporation, Inc.
 *
 * Authors:
 *   Tim Whisonant <tim.whisonant@intel.com>
 *   Ananda Ravuri <ananda.ravuri@intel.com>
 *   Russell H. Weight <russell.h.weight@intel.com>
 */

#ifndef _UAPI_LINUX_HE_CACHE_DFL_H
#define _UAPI_LINUX_HE_CACHE_DFL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define DFL_HE_CACHE_API_VERSION 0

/*
 * The IOCTL interface for DFL based HE CACHE is designed for extensibility by
 * embedding the structure length (argsz) and flags into structures passed
 * between kernel and userspace. This design referenced the VFIO IOCTL
 * interface (include/uapi/linux/vfio.h).
 */

#define DFL_HE_CACHE_MAGIC 0xB6

#define DFL_HE_CACHE_BASE 0

/**
 * DFL_HE_CACHE_GET_API_VERSION - _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 0)
 *
 * Report the version of the driver API.
 * Return: Driver API Version.
 */

#define DFL_HE_CACHE_GET_API_VERSION                                           \
  _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 0)

/**
 * DFL_HE_CACHE_CHECK_EXTENSION - _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 1)
 *
 * Check whether an extension is supported.
 * Return: 0 if not supported, otherwise the extension is supported.
 */

#define DFL_HE_CACHE_CHECK_EXTENSION                                           \
  _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 1)

/**
 * DFL_HE_CACHE_GET_REGION_INFO - _IOWR(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE +
 * 2, struct dfl_he_cache_region_info)
 *
 * Retrieve information about a device memory region.
 * Caller provides struct dfl_he_cache_region_info with flags.
 * Driver returns the region info in other fields.
 * Return: 0 on success, -errno on failure.
 */

#define DFL_HE_CACHE_GET_REGION_INFO                                           \
  _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 2)

struct dfl_he_cache_region_info {
  /* Input */
  __u32 argsz; /* Structure length */
  /* Output */
  __u32 flags;                             /* Access permission */
#define DFL_HE_CACHE_REGION_READ (1 << 0)  /* Region is readable */
#define DFL_HE_CACHE_REGION_WRITE (1 << 1) /* Region is writable */
#define DFL_HE_CACHE_REGION_MMAP (1 << 2)  /* Can be mmaped to userspace */
  __u64 size;                              /* Region size (bytes) */
  __u64 offset; /* Region offset from start of device fd */
};

/**
* DFL_HE_CACHE_NUMA_DMA_MAP - _IOWR(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 3,
*                                      struct dfl_he_cache_dma_map)
*
* Map the dma memory per user_addr,length and numa node which are provided by
caller.
* The driver allocates memory on the numa node, converts the user's virtual
address
* to a continuous physical address, and writes the physical address to
* the host executor's read/write address table CSR.

* This interface only accepts page-size aligned user memory for dma mapping.
* Return: 0 on success, -errno on failure.
*/

#define DFL_ARRAY_MAX_SIZE 0x10

#define DFL_HE_CACHE_NUMA_DMA_MAP _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 3)

struct dfl_he_cache_dma_map {
  /* Input */
  __u32 argsz;                         /* Structure length */
  __u32 flags;                         /* flags */
  __u64 user_addr;                     /* Process virtual address */
  __u64 length;                        /* Length of mapping (bytes)*/
  __u32 numa_node;                     /* Node 0,1 2 */
  __u64 csr_array[DFL_ARRAY_MAX_SIZE]; /* CSR  */
};

/**
 * DFL_HE_CACHE_NUMA_DMA_UNMAP - _IOWR(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE +
 * 4, struct dfl_he_cache_dma_unmap)
 *
 * Unmpas the dma memory per user_addr and length which are provided by caller.
 * The driver deletes the physical pages of the user address and writes a zero
 * to the read/write address table CSR.
 * Return: 0 on success, -errno on failure.
 */

#define DFL_HE_CACHE_NUMA_DMA_UNMAP                                            \
  _IO(DFL_HE_CACHE_MAGIC, DFL_HE_CACHE_BASE + 4)

struct dfl_he_cache_dma_unmap {
  /* Input */
  __u32 argsz;                         /* Structure length */
  __u32 flags;                         /* flags */
  __u64 user_addr;                     /* Process virtual address */
  __u64 length;                        /* Length of mapping (bytes)*/
  __u64 csr_array[DFL_ARRAY_MAX_SIZE]; /* CSR */
};

#endif /* _UAPI_LINUX_HE_CACHE_DFL_H */
