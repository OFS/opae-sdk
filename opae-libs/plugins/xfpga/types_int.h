// Copyright(c) 2017-2020, Intel Corporation
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
#include <limits.h>
#include <pthread.h>
#include <opae/types.h>
#include <opae/sysobject.h>
#include <opae/types_enum.h>
#include <opae/metrics.h>
#include "metrics/vector.h"

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
#define SYSFS_FPGA_FMT "/intel-fpga-dev.%d"

// FPGA device id
#define FPGA_SYSFS_DEVICEID "device/device"

// FME path
#define SYSFS_FME_PATH         "*%d/*-fme.%d"

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)

// FPGA token magic (FPGATOKN)
#define FPGA_TOKEN_MAGIC 0x46504741544f4b4e
// FPGA handle magic (FPGAHNDL)
#define FPGA_HANDLE_MAGIC 0x46504741484e444c
// FPGA property magic (FPGAPROP)
#define FPGA_PROPERTY_MAGIC 0x4650474150524f50
// FPGA event handle magid (FPGAEVNT)
#define FPGA_EVENT_HANDLE_MAGIC 0x4650474145564e54
// FPGA invalid magic (FPGAINVL)
#define FPGA_INVALID_MAGIC 0x46504741494e564c

// Register/Unregister for interrupts
#define FPGA_IRQ_ASSIGN (1 << 0)
#define FPGA_IRQ_DEASSIGN (1 << 1)

// Get file descriptor from event handle
#define FILE_DESCRIPTOR(eh) (((struct _fpga_event_handle *)eh)->fd)
#ifdef __cplusplus
extern "C" {
#endif
/** System-wide unique FPGA resource identifier */
struct _fpga_token {
	uint32_t device_instance;
	uint32_t subdev_instance;
	uint64_t magic;
	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
	struct error_list *errors;
};

enum fpga_hw_type {
	FPGA_HW_MCP,
	FPGA_HW_DCP_RC,
	FPGA_HW_DCP_D5005,
	FPGA_HW_DCP_N3000,
	FPGA_HW_UNKNOWN
};

// FPGA enum metrics struct
struct _fpga_enum_metric {

	char group_name[FPGA_METRIC_STR_SIZE];            // Metrics Group name
	char group_sysfs[FPGA_METRIC_STR_SIZE];           // Metrics Group sysfs path

	char metric_name[FPGA_METRIC_STR_SIZE];           // Metrics name
	char metric_sysfs[FPGA_METRIC_STR_SIZE];          // Metrics sysfs path

	char qualifier_name[FPGA_METRIC_STR_SIZE];        // Metrics qualifier name

	char metric_units[FPGA_METRIC_STR_SIZE];          // Metrics units

	uint64_t metric_num;                              // Metrics ID

	enum fpga_metric_datatype metric_datatype;        // Metrics datatype

	enum fpga_metric_type metric_type;                // Metric type

	enum fpga_hw_type hw_type;                       // Hardware type

	uint64_t mmio_offset;                            // AFU Metric BBS mmio offset

};


struct _fpga_bmc_metric {

	char group_name[FPGA_METRIC_STR_SIZE];     // Metrics Group name
	char metric_name[FPGA_METRIC_STR_SIZE];    // Metrics  name
	struct fpga_metric fpga_metric;             // Metric value
};

/** Process-wide unique FPGA handle */
struct _fpga_handle {
	pthread_mutex_t lock;
	uint64_t magic;
	fpga_token token;

	int fddev;                      // file descriptor for the device.
	int fdfpgad;                    // file descriptor for the event daemon.
	uint32_t num_irqs;              // number of interrupts supported
	uint32_t irq_set;               // bitmask of irqs set
	struct wsid_tracker *wsid_root; // wsid information (list)
	struct wsid_tracker *mmio_root; // MMIO information (list)
	void *umsg_virt;	        // umsg Virtual Memory pointer
	uint64_t umsg_size;	        // umsg Virtual Memory Size
	uint64_t *umsg_iova;	        // umsg IOVA from driver

	// Metric
	bool metric_enum_status;                             // metric enum status
	fpga_metric_vector fpga_enum_metric_vector;          // metric enum vector
	void *bmc_handle;                                    // bmc module handle
	struct _fpga_bmc_metric *_bmc_metric_cache_value;    // bmc cache values
	uint64_t num_bmc_metric;                             // num of bmc values
#define OPAE_FLAG_HAS_MMX512 (1u << 0)
	uint32_t flags;
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
	uint64_t wsid;
	uint64_t addr;
	uint64_t phys;
	uint64_t len;
	uint64_t offset;
	uint32_t index;
	int flags;
	struct wsid_map *next;
};

/*
 * Hash table to store wsid_maps
 */
struct wsid_tracker {
	uint64_t          n_hash_buckets;
	struct wsid_map **table;
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

typedef enum {
	FPGA_SYSFS_DIR = 0,
	FPGA_SYSFS_LIST,
	FPGA_SYSFS_FILE
} fpga_sysfs_type;

struct _fpga_object {
	pthread_mutex_t lock;
	fpga_handle handle;
	fpga_sysfs_type type;
	char *path;
	char *name;
	int perm;
	size_t size;
	size_t max_size;
	uint8_t *buffer;
	fpga_object *objects;
};

typedef char max_path_t[PATH_MAX];

#ifdef __cplusplus
}
#endif
#endif // __FPGA_TYPES_INT_H__
