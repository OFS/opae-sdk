// Copyright(c) 2018, Intel Corporation
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
/*
 * @file sysinfo.h
 *
 * @brief
 */
#ifndef SYSINFO_H
#define SYSINFO_H

#include <opae/fpga.h>

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
#define SYSFS_FPGA_FMT "/intel-fpga-dev.%d"

// FPGA device id
#define FPGA_SYSFS_DEVICEID "device/device"

// Integrated FPGA Device ID
#define FPGA_INTEGRATED_DEVICEID 0xbcc0

// Discrete FPGA Device ID
#define FPGA_DISCRETE_DEVICEID 0x09c4

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)

#define DEV_PATH_MAX 256

// SYSFS FME Errors
#define FME_SYSFS_FME_ERRORS "errors/fme-errors/errors"
#define FME_SYSFS_PCIE0_ERRORS "errors/pcie0_errors"
#define FME_SYSFS_PCIE1_ERRORS "errors/pcie1_errors"

#define FME_SYSFS_BBS_ERRORS "errors/bbs_errors"
#define FME_SYSFS_GBS_ERRORS "errors/gbs_errors"
#define FME_SYSFS_WARNING_ERRORS "errors/warning_errors"

#define FME_SYSFS_NONFATAL_ERRORS "errors/nonfatal_errors"
#define FME_SYSFS_CATFATAL_ERRORS "errors/catfatal_errors"
#define FME_SYSFS_INJECT_ERROR "errors/inject_error"

#define FME_SYSFS_ERR_REVISION "errors/revision"

#define PORT_SYSFS_ERR "errors/errors"
#define PORT_SYSFS_ERR_CLEAR "errors/clear"

// SYFS Thermal
#define FME_SYSFS_THERMAL_MGMT_TEMP "thermal_mgmt/temperature"
#define FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP "thermal_mgmt/threshold_trip"

// SYSFS Power
#define FME_SYSFS_POWER_MGMT_CONSUMED "power_mgmt/consumed"

// MMIO scratchpad
#define PORT_SCRATCHPAD0 0x0028
#define NLB_CSR_SCRATCHPAD (0x40000 + 0x0104)
#define PORT_MMIO_LEN (0x40000 + 0x0512)

#define MMO_WRITE64_VALUE 0xF1F1F1F1F1F1F1F1
#define MMO_WRITE32_VALUE 0xF1F1F1

#define FPGA_CSR_LEN 64

#define DEVICEID_PATH "/sys/bus/pci/devices/%04x:%02x:%02x.%d/device"
#define FPGA_PORT_RES_PATH "/sys/bus/pci/devices/%04x:%02x:%02x.%d/resource2"


#define FPGA_SET_BIT(val, index) (val |= (1 << index))
#define FPGA_CLEAR_BIT(val, index) (val &= ~(1 << index))
#define FPGA_TOGGLE_BIT(val, index) (val ^= (1 << index))
#define FPGA_BIT_IS_SET(val, index) (((val) >> (index)) & 1)

#ifndef CL
#define CL(x) ((x)*64)
#endif // CL
#ifndef LOG2_CL
#define LOG2_CL 6
#endif // LOG2_CL
#ifndef MB
#define MB(x) ((x)*1024 * 1024)
#endif // MB

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

#define LPBK1_BUFFER_SIZE MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE MB(2)
#define LPBK1_DSM_SIZE MB(2)
#define CSR_SRC_ADDR 0x0120
#define CSR_DST_ADDR 0x0128
#define CSR_CTL 0x0138
#define CSR_CFG 0x0140
#define CSR_NUM_LINES 0x0130
#define DSM_STATUS_TEST_COMPLETE 0x40
#define CSR_AFU_DSM_BASEL 0x0110
#define CSR_AFU_DSM_BASEH 0x0114

struct dev_list {
	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
	fpga_objtype objtype;
	fpga_guid guid;
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t socket_id;
	uint16_t vendor_id;
	uint16_t device_id;

	uint32_t fpga_num_slots;
	uint64_t fpga_bitstream_id;
	fpga_version fpga_bbs_version;

	struct dev_list *next;
	struct dev_list *parent;
	struct dev_list *fme;
};

#ifdef __cplusplus
extern "C" {
#endif

fpga_result fpgainfo_enumerate_devices(struct dev_list *head);
fpga_result fpgainfo_sysfs_read_u32(const char *path, uint32_t *u);
fpga_result fpgainfo_sysfs_read_u64(const char *path, uint64_t *u);
const char *get_sysfs_path(fpga_properties props, fpga_objtype objtype,
			   struct dev_list **item);
fpga_result glob_sysfs_path(char *path);

#ifdef __cplusplus
}
#endif

#endif /* !SYSINFO_H */
