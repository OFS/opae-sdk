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
 * @file opae/uio.h
 * @brief APIs to manage a PCIe device via UIO.
 *
 * Presents a simple interface for interacting with a DFL device that is
 * bound to its UIO driver.
 * See https://kernel.org/doc/html/v4.14/driver-api/uio-howto.html for a
 * description of UIO.
 *
 * Provides APIs for opening/closing the device and for querying info about
 * the MMIO regions of the device.
 */

#ifndef __OPAE_UIO_H__
#define __OPAE_UIO_H__

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#define OPAE_UIO_PATH_MAX 256

/**
 * MMIO region info
 *
 * Describes a range of mappable MMIO.
 */
struct opae_uio_device_region {
	uint32_t region_index;
	uint8_t *region_ptr;
	size_t region_page_offset;
	size_t region_size;
	struct opae_uio_device_region *next;
};

/**
 * OPAE UIO device abstraction
 *
 * This structure is used to interact with the OPAE UIO API. It tracks
 * Each UIO device has a file descriptor that is used to mmap its regions
 * into user address space. Each device also has one or more MMIO regions.
 */
struct opae_uio {
	char device_path[OPAE_UIO_PATH_MAX];
	int device_fd;
	struct opae_uio_device_region *regions;
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open and populate a UIO device
 *
 * Opens the Device Feature List device corresponding to the device name
 * given in dfl_device, eg "dfl_dev.10". The device must be bound to the
 * dfl-uio-pdev driver prior to opening it. The data structures corresponding
 * to the MMIO regions are initialized.
 *
 * @param[out] u         Storage for the device. May be stack-resident.
 * @param[in] dfl_device The name of the desired DFL device.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.sh}
 * $ sudo
 * @endcode
 *
 * Example
 * @code{.c}
 * struct opae_uio u;
 *
 * if (opae_uio_open(&u, "dfl_dev.10")) {
 *   // handle error
 * }
 * @endcode
 */
int opae_uio_open(struct opae_uio *u,
		  const char *dfl_device);

/**
 * Query device MMIO region
 *
 * Retrieves info describing the MMIO region with the given region index.
 * The device structure u must have been previously opened by a call to
 * opae_uio_open.
 *
 * @param[in] u     The open OPAE UIO device.
 * @param[in] index The zero-based index of the desired MMIO region.
 * @param[out] ptr  Optional pointer to receive the virtual address for
 *                  the region. Pass NULL to ignore.
 * @param[out] size Optional pointer to receive the size of the MMIO region.
 *                  Pass NULL to ignore.
 * @returns Non-zero on error (including index out-of-range). Zero on success.
 *
 * Example
 * @code{.c}
 * struct opae_uio u;
 *
 * uint8_t *virt = NULL;
 * size_t size = 0;
 *
 * if (opae_uio_open(&u, "dfl_dev.10")) {
 *   // handle error
 * } else {
 *   opae_uio_region_get(&u, 0, &virt, &size);
 * }
 * @endcode
 */
int opae_uio_region_get(struct opae_uio *u,
			uint32_t index,
			uint8_t **ptr,
			size_t *size);

/**
 * Release and close a UIO device
 *
 * The given device pointer must have been previously initialized by
 * opae_uio_open. Releases all data structures.
 *
 * @param[in] u Storage for the device info. May be stack-resident.
 *
 * Example
 * @code{.c}
 * struct opae_uio u;
 *
 * if (opae_uio_open(&u, "dfl_dev.10")) {
 *   // handle error
 * } else {
 *   // interact with the device
 *   ...
 *   // free the device
 *   opae_uio_close(&u);
 * }
 * @endcode
 *
 * Example
 * @code{.sh}
 * $ sudo python3
 * @endcode
 */
void opae_uio_close(struct opae_uio *u);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __OPAE_UIO_H__
