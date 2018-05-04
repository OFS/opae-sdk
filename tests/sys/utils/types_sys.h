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

/**
 * \file types_int.h
 * \brief Internal type definitions for FPGA API
 */

#ifndef __FPGA_TYPES_INT_H__
#define __FPGA_TYPES_INT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <opae/types.h>
#include <opae/types_enum.h>

#define SYSFS_PATH_MAX 256
#define SYSFS_FPGA_CLASS_PATH "/sys/class/fpga"
#define FPGA_DEV_PATH "/dev"

#define SYSFS_AFU_PATH_FMT "/intel-fpga-dev.%d/intel-fpga-port.%d"
#define SYSFS_FME_PATH_FMT "/intel-fpga-dev.%d/intel-fpga-fme.%d"

// substring that identifies a sysfs directory as the FME device.
#define FPGA_SYSFS_FME "fme"
// substring that identifies a sysfs directory as the AFU device.
#define FPGA_SYSFS_AFU "port"
// name of the FME interface ID (GUID) sysfs node.
#define FPGA_SYSFS_FME_INTERFACE_ID "pr/interface_id"
// name of the AFU GUID sysfs node.
#define FPGA_SYSFS_AFU_GUID "afu_id"
// name of the socket id sysfs node.
#define FPGA_SYSFS_SOCKET_ID "socket_id"
// name of the number of slots sysfs node.
#define FPGA_SYSFS_NUM_SLOTS "ports_num"
// name of the bitstream id sysfs node.
#define FPGA_SYSFS_BITSTREAM_ID "bitstream_id"

// fpga device path
#define SYSFS_FPGA_FMT                  "/intel-fpga-dev.%d"

// FPGA device id
#define FPGA_SYSFS_DEVICEID            "device/device"

// Integrated FPGA Device ID
#define FPGA_INTEGRATED_DEVICEID             0xbcc0

// Discrete FPGA Device ID
#define FPGA_DISCRETE_DEVICEID               0x09c4

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)

#define DEV_PATH_MAX 256

// FPGA token magic (FPGATOKN)
#define FPGA_TOKEN_MAGIC    0x46504741544f4b4e
// FPGA handle magic (FPGAHNDL)
#define FPGA_HANDLE_MAGIC   0x46504741484e444c
// FPGA property magic (FPGAPROP)
#define FPGA_PROPERTY_MAGIC 0x4650474150524f50
//FPGA event handle magid (FPGAEVNT)
#define FPGA_EVENT_HANDLE_MAGIC 0x4650474145564e54
// FPGA invalid magic (FPGAINVL)
#define FPGA_INVALID_MAGIC  0x46504741494e564c

//Register/Unregister for interrupts
#define FPGA_IRQ_ASSIGN    (1 << 0)
#define FPGA_IRQ_DEASSIGN  (1 << 1)

//Get file descriptor from event handle
#define FILE_DESCRIPTOR(eh) (((struct _fpga_event_handle *)eh)->fd)

/** System-wide unique FPGA resource identifier */
struct _fpga_token {
	uint64_t magic;
	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
};

/** Process-wide unique FPGA handle */
struct _fpga_handle {
	pthread_mutex_t lock;
	uint64_t magic;
	fpga_token token;
	int fddev;                  // file descriptor for the device.
	int fdfpgad;                // file descriptor for the event daemon.
	struct wsid_map *wsid_root; // wsid information (list)
	struct wsid_map *mmio_root; // MMIO information (list)
	void *umsg_virt;	    // umsg Virtual Memory pointer
	uint64_t umsg_size;	    // umsg Virtual Memory Size
	uint64_t *umsg_iova;	    // umsg IOVA from driver
};

/** Object property struct
    Intent is for property struct to be created dynamically */
struct _fpga_properties {
	pthread_mutex_t lock;
	uint64_t magic;
	/* Common properties */
	uint64_t valid_fields; // bitmap of valid fields
	// valid here means the field has been set using the API
	// bit 0x00 - parent field is valid
	// bit 0x01 - objtype field is valid
	// bit 0x02 - bus field is valid
	// ...
	// up to bit 0x1F
	fpga_guid guid;		// Applies only to accelerator types
	fpga_token parent;
	fpga_objtype objtype;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t socket_id;
	uint64_t object_id;
	// TODO uint16_t device_id;

	/* Object-specific properties
	 * bitfields start as 0x20
	 */
	union {

		/* fpga object properties
		 * */
		struct {
			uint32_t num_slots;
			uint64_t bbs_id;
			fpga_version bbs_version;
			// TODO uint16_t vendor_id;
			// TODO char model[FPGA_MODEL_LENGTH];
			// TODO uint64_t local_memory_size;
			// TODO uint64_t capabilities; #<{(| bitfield (HSSI, iommu, ...) |)}>#
		} fpga;

		/* accelerator object properties
		 * */
		struct {
			fpga_accelerator_state state;
			uint32_t num_mmio;
			uint32_t num_interrupts;
		} accelerator;

	} u;

};

/*
 * Event handle struct to perform
 * event operations
 *
 */
struct _fpga_event_handle {
	pthread_mutex_t lock;
	uint64_t magic;
	int fd;
	uint32_t flags;
};

/*
 * Global list to store wsid/physptr/length vectors
 */
struct wsid_map {
	uint64_t         wsid;
	uint64_t         addr;
	uint64_t         phys;
	uint64_t         len;
	uint64_t         offset;
	uint32_t         index;
	int              flags;
	struct wsid_map *next;
};

/*
 * Global list to store tokens received during enumeration
 * Since tokens as seen by the API are only void*, we need to keep the actual
 * structs somewhere.
 */
struct token_map {
	struct _fpga_token _token;
	struct token_map *next;
};


#endif // __FPGA_TYPES_INT_H__
